#include "identifier.h"

#include "tty.h"
#include "../../statement/scope.h"
#include "../../type/types.h"
#include "parser/literal/wrapper.h"
#include "parser/type/clash_types.h"
#include "parser/type/stringify_type.h"

IdentifierInfo new_identifier(Token base_identifier, Parser* parser, const unsigned flags) {
    bool is_external = false;
    if(streq(base_identifier.trace.source, str("extern"))) {
        is_external = true;
        base_identifier = expect(parser->tokenizer, TokenIdentifier);
    }

    IdentifierInfo info = {
        .identifier = {
            .base = base_identifier.trace.source,
            .parent_scope = last(parser->stack),
            .is_external = is_external,
        },
        .value = find_on_stack(parser->stack, base_identifier.trace),
        .declaration_scope = last(parser->stack),
        .trace = base_identifier.trace,
    };

    Scope* outer_scope = info.declaration_scope;

compound_start:
    if(flags & IdentifierDeclaration
       && !(info.value && info.value->Variable.declaration->id == NodeStructDeclaration)) {
        if(info.value && outer_scope != info.declaration_scope
           && info.value->Variable.declaration->id == NodeFunctionDeclaration
           && info.declaration_scope->declaration->id == NodeStructDeclaration
           && info.declaration_scope->declaration->type->StructType.is_trait) {
            apply_type_arguments(info.value, parser, true);
        }

        info.generics_collection = collect_generics(parser);
    } else if(info.value) {
        apply_type_arguments(info.value, parser, false);
    }

    if(!try(parser->tokenizer, TokenDoubleColon, NULL)) {
        if(!(flags & IdentifierDeclaration) || outer_scope->declaration->id != NodeStructDeclaration
           || outer_scope == info.declaration_scope) {
            return info;
        }

        if(info.declaration_scope->declaration->id != NodeStructDeclaration
           || !info.declaration_scope->declaration->type->StructType.is_trait) {
            push(parser->tokenizer->messages,
                 MERROR(info.trace, str("non-trait in external declaration inside of a structure")));
            return info;
        }

        if(!info.value) {
            String message = strf(0, iftty(HERR"%.*s"H" is not a function on trait object "HERR,
                                      "%.*s is not a function on trait object "),
                                  fmtof(info.identifier.base)).as_owned;
            stringify_type(info.declaration_scope->declaration->type, &message, 0, 0);
            push(parser->tokenizer->messages, MERROR(info.trace, strf(&message, iftty(H, ""))));
            info.declaration_scope = outer_scope;
            return info;
        }

        info.identifier.trait = (void*) info.declaration_scope->declaration;
        StructType* const outer_struct_type = (void*) outer_scope->declaration->type;
        const str trait_identifier = info.identifier.trait->identifier.base;
        Scope* trait_fields = NULL;

        Vec(Action) actions = extract_actions((void*) info.value, true);
        for(u32 i = 0; i < len(actions); i++) {
            push(&info.declaration_actions, actions[i]);
        }

        if(!((trait_fields = get(outer_struct_type->traits, trait_identifier)))) {
            put(&outer_struct_type->traits, trait_identifier, { NodeScope });
            trait_fields = get(outer_struct_type->traits, trait_identifier);
            trait_fields->declaration = outer_scope->declaration;
            trait_fields->result_value = (void*) info.parent_value;
        } else {
            clash_types((void*) trait_fields->result_value, (void*) info.parent_value, info.parent_value->trace,
                        parser->tokenizer->messages, 0);
        }

        info.trait_scope = trait_fields;
        info.declaration_scope = outer_scope;
        return info;
    }

    if(!info.value || !info.value->Variable.declaration->const_value) {
        return info;
    }

    Vec(Action) actions = extract_actions((void*) info.value, true);
    for(u32 i = 0; i < len(actions); i++)
        push(&info.declaration_actions, actions[i]);

    if(try(parser->tokenizer, '[', NULL)) {
        IdentifierInfo trait_info = new_identifier(expect(parser->tokenizer, TokenIdentifier), parser, flags);
        expect(parser->tokenizer, ']');

        if(info.value->Variable.declaration->id != NodeStructDeclaration) {
            push(parser->tokenizer->messages, MERROR(info.trace,
                     str("attempting to assign trait declaration to a non-struct value")));
            goto compound_start;
        }

        outer_scope = info.value->Variable.declaration->type->StructType.module->scope;
        for(u32 i = 0; i < len(info.declaration_actions); i++) {
            push(&trait_info.declaration_actions, actions[i]);
        }

        info = trait_info;
        info.identifier.parent_scope = outer_scope;
        goto compound_start;
    }

    Module* module = NULL;
    switch(info.value->Variable.declaration->const_value->id) {
        case NodeStructType:
            module = info.value->Variable.declaration->const_value->StructType.module;
            break;

        case NodeModule:
            module = (void*) info.value->Variable.declaration->const_value;
            break;

        default:
            push(parser->tokenizer->messages, MERROR(info.value->trace,
                     strf(0, iftty("'\33[36m%.*s\33[0m' is not a module", "'%.*s' is not a module"),
                         fmtof(info.value->trace.source))));
    }

    const Trace next_trace = expect(parser->tokenizer, TokenIdentifier).trace;
    const Action wrapper_action = info.value->action;

    if(!module) {
        goto compound_start;
    }

    info.declaration_scope = module->scope;
    info.parent_value = info.value;
    info.value = find_in_scope(*module->scope, next_trace);

    if(info.value && wrapper_action.type) {
        assign_action((void*) info.value, wrapper_action, false, true);
    }

    info.trace = stretch(info.trace, next_trace);
    info.identifier.base = next_trace.source;
    info.identifier.parent_declaration = module->declaration;

    if(module->declaration && module->declaration->id == NodeStructDeclaration
       && !(module->declaration->type->StructType.is_trait && info.value)) {
        info.identifier.parent_scope = module->scope;
    }

    goto compound_start;
}
