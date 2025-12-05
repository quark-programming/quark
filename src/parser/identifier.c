#include "generics.c"

typedef struct {
	Identifier* identifier;
	Wrapper* value;
	Scope* scope;
	Trace trace;
	GenericsCollection generics_collection;
} IdentifierInfo;

IdentifierInfo new_identifier(Token base_identifier, Parser* parser) {
	unsigned long identifier_flags = 0;
	if(streq(base_identifier.trace.slice, str("extern"))) {
		identifier_flags |= fExternal;
		base_identifier = expect(parser->tokenizer, TokenIdentifier);
	}

	IdentifierInfo info = {
		.identifier = (void*) new_node((Node) { .Identifier = {
				.compiler = (void*) &comp_Identifier,
				.flags = identifier_flags,
				.base = base_identifier.trace.slice,
				.parent = last(parser->stack)->parent,
		}}),
		.value = find(parser->stack, base_identifier.trace),
		.scope = last(parser->stack),
		.trace = base_identifier.trace,
	};

compound_start:
	if(info.value) assign_generics(info.value, parser);
	else info.generics_collection = collect_generics(parser);

	if(!info.value || !info.value->ref->Declaration.const_value
			|| info.value->ref->Declaration.const_value->compiler != (void*) &comp_StructType
			|| !try(parser->tokenizer, TokenDoubleColon, NULL)) {
		return info;
	}

	StructType* const parent_struct = (void*) info.value->ref->Declaration.const_value;
	const Trace next_trace = expect(parser->tokenizer, TokenIdentifier).trace;
	const TypeStateAction wrapper_action = info.value->action;
	info.value = get(*(info.scope = parent_struct->body), next_trace);

	if(info.value && wrapper_action.type) {
		info.value = (void*) new_node((Node) { .Wrapper = {
				.compiler = (void*) &comp_Wrapper,
				.trace = info.value->trace,
				.flags = info.value->flags,
				.type = new_type((Type) { .Wrapper = {
						.compiler = (void*) &comp_Wrapper,
						.trace = info.value->type->trace,
						.flags = info.value->type->flags,
						.ref = (void*) info.value->type,
						.action = wrapper_action,
				}}),
				.ref = (void*) info.value,
				.action = wrapper_action,
		}});
	}

	info.trace = stretch(info.trace, next_trace);
	info.identifier->base = next_trace.slice;
	info.identifier->parent = (void*) parent_struct;

	goto compound_start;
}
