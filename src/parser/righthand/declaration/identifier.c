#include "identifier.h"
#include "../../statement/scope.h"

IdentifierInfo new_identifier(Token base_identifier, Parser* parser, const unsigned flags) {
    bool external = false;
    if(streq(base_identifier.trace.source, String("extern"))) {
        external = true;
        base_identifier = expect(parser->tokenizer, TokenIdentifier);
    }

    IdentifierInfo info = {
        .identifier = {
            .base = base_identifier.trace.source,
            .parent_scope = last(parser->stack)->parent,
            .external = external,
        },
        .value = flags & IdentifierDeclaration ? NULL : find_on_stack(parser->stack, base_identifier.trace),
        .declaration_scope = last(parser->stack),
        .trace = base_identifier.trace,
    };

compound_start:
    if(info.value) {
        assign_generics(info.value, parser);
    } else if(flags & IdentifierDeclaration) {
        info.generics_collection = collect_generics(parser);
    }

    if(!info.value || !info.value->Variable.declaration->const_value
       || info.value->Variable.declaration->const_value->id != NodeStructType
       || !try(parser->tokenizer, TokenDoubleColon, NULL)
    ) {
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
