#include "identifier.h"
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
            .parent_scope = last(parser->stack)->parent,
            .is_external = is_external,
        },
        .value = find_on_stack(parser->stack, base_identifier.trace),
        .declaration_scope = last(parser->stack),
        .trace = base_identifier.trace,
    };

    Scope* const initial_declaration_scope = info.declaration_scope;

compound_start:
    if(flags & IdentifierDeclaration) {
        info.generics_collection = collect_generics(parser);
    } else if(info.value) {
        apply_type_arguments(info.value, parser);
    }

    if(!info.value || !info.value->Variable.declaration->const_value
       || info.value->Variable.declaration->const_value->id != NodeStructType
       || !try(parser->tokenizer, TokenDoubleColon, NULL)) {
        if(initial_declaration_scope->parent->id == NodeStructType
           && initial_declaration_scope != info.declaration_scope) {
            info.identifier.reference_structure = (void*) info.declaration_scope->parent;

            StructType* const wrapped_structure = (void*) initial_declaration_scope->parent;
            str const reference_identifier = info.identifier.reference_structure->parent->identifier.base;
            Scope* reference_declarations = NULL;

            if(!((reference_declarations = get(wrapped_structure->reference_structures, reference_identifier)))) {
                Scope scope = { .id = NodeScope };

                put(&wrapped_structure->reference_structures, reference_identifier, scope);
                reference_declarations = get(wrapped_structure->reference_structures, reference_identifier);
                scope.parent = (void*) reference_declarations;
            }

            info.declaration_scope = reference_declarations;
        }

        return info;
    }

    StructType* const parent_struct = (void*) info.value->Variable.declaration->const_value;
    const Trace next_trace = expect(parser->tokenizer, TokenIdentifier).trace;
    const Action wrapper_action = info.value->action;

    info.declaration_scope = parent_struct->static_body;
    info.value = find_in_scope(*parent_struct->static_body, next_trace);

    if(info.value && wrapper_action.type) {
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
    info.identifier.parent_declaration = (void*) parent_struct;

    goto compound_start;
}
