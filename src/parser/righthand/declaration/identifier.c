#include "identifier.h"

#include "tty.h"
#include "../../statement/scope.h"
#include "../../type/types.h"

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

    Scope* const outer_scope = info.declaration_scope;

compound_start:
    if(flags & IdentifierDeclaration) {
        info.generics_collection = collect_generics(parser);
    } else if(info.value) {
        apply_type_arguments(info.value, parser);
    }

    if(!info.value || !info.value->Variable.declaration->const_value) {
        return info;
    }

    if(!try(parser->tokenizer, TokenDoubleColon, NULL)) {
        if(outer_scope->declaration->id != NodeStructType || outer_scope == info.declaration_scope) {
            return info;
        }

        if(info.declaration_scope->declaration->id != NodeStructDeclaration
           || !info.declaration_scope->declaration->type->StructType.is_trait) {
            push(parser->tokenizer->messages,
                 MERROR(info.trace, str("non-trait in external declaration inside of a structure")));
            return info;
        }

        info.identifier.trait = (void*) info.declaration_scope->declaration;
        StructType* const outer_struct_type = (void*) outer_scope->declaration->type;
        const str trait_identifier = info.identifier.trait->identifier.base;
        Scope* trait_fields = NULL;

        if(!((trait_fields = get(outer_struct_type->traits, trait_identifier)))) {
            put(&outer_struct_type->traits, trait_identifier, { NodeScope });
            trait_fields = get(outer_struct_type->traits, trait_identifier);
            trait_fields->declaration = outer_scope->declaration;
        }

        info.declaration_scope = trait_fields;
        return info;
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
    info.value = find_in_scope(*module->scope, next_trace);

    if(info.value && wrapper_action.type) {
        // TODO: (fix) this action can be overridden on next compound_start
        info.value->action = wrapper_action;
        info.value->type = new_type((Type) {
            .Wrapper = {
                .id = WrapperAuto,
                .trace = info.value->type->trace,
                .flags = info.value->type->flags,
                .Auto.ref = info.value->type,
                .action = wrapper_action,
            },
        });
    }

    info.trace = stretch(info.trace, next_trace);
    info.identifier.base = next_trace.source;
    info.identifier.parent_declaration = module->declaration;

    goto compound_start;
}
