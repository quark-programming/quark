#include "parser.c"

Type* clone_base_type(Type* type);
NodeList collect_until(Parser* parser, Node* (*supplier)(Parser*),
		unsigned char divider, unsigned char terminator);
Node* expression(Parser* parser);
Message see_declaration(Declaration* declaration, Node* node);
int clash_types(Type* a, Type* b, Trace trace, Messages* messages, unsigned flags);
void stringify_type(Type* type, str* string, unsigned flags);

Type* wrap_applied_generics(Type* type, TypeList generics, Declaration* declaration) {
	return new_type((Type) { .Wrapper = {
			.compiler = (void*) &comp_Wrapper,
			.trace = type->trace,
			.flags = type->flags,
			.action = { StateActionGenerics, generics, (void*) declaration },
			.ref = (void*) type,
	}});
}

int collecting_type_arguments = 0;

void assign_generics(Wrapper* variable, Parser* parser) {
	while(!variable->variable) variable = (void*) variable->ref;
	Declaration* const declaration = (void*) variable->ref;
	if(!declaration->generics.base.size) return;
	const TypeList base_generics = declaration->generics.base;

	TypeList input_generics = { 0 };
	for(size_t i = 0; i < base_generics.size; i++) {
		push(&input_generics, new_type((Type) { .Wrapper = {
					.compiler = (void*) &comp_Wrapper,
					.trace = variable->trace,
					.flags = base_generics.data[i]->flags,
					.compare = base_generics.data[i],
		}}));
	}

	if(try(parser->tokenizer, '<', 0)) {
		collecting_type_arguments = 1;
		NodeList type_arguments = collect_until(parser, &expression, ',', '>');
		collecting_type_arguments = 0;
		for(size_t i = 0; i < type_arguments.size; i++) {
			if(!(type_arguments.data[i]->flags & fType)) {
				push(parser->tokenizer->messages, Err(type_arguments.data[i]->trace,
							str("expected a type in type arguments")));
			}

			if(i >= base_generics.size) {
				push(parser->tokenizer->messages, Err(stretch(type_arguments.data[i]->trace,
								last(type_arguments)->trace), str("too many type arguments")));
				push(parser->tokenizer->messages, see_declaration(declaration,
							type_arguments.data[i]));
				break;
			}

			clash_types(input_generics.data[i], (void*) type_arguments.data[i],
					type_arguments.data[i]->trace, parser->tokenizer->messages, 0);
			input_generics.data[i]->trace = type_arguments.data[i]->trace;
		}
	}

	variable->action = (TypeStateAction) {
		StateActionGenerics, input_generics, (void*) declaration
	};
	variable->type = wrap_applied_generics(variable->type, input_generics, declaration);
}

typedef Vector(Declaration**) DeclarationSetters;

typedef struct {
	TypeList base_generics;
	Scope* scope;
	DeclarationSetters declaration_setters;
} GenericsCollection;

GenericsCollection collect_generics(Parser* parser) {
	if(!try(parser->tokenizer, '<', 0)) return (GenericsCollection) { 0 };

	TypeList base_generics = { 0 };
	DeclarationSetters declaration_setters = { 0 };
	Scope* scope = new_scope(NULL);

	while(parser->tokenizer->current.type
			&& parser->tokenizer->current.type != '>') {
		Token identifier = expect(parser->tokenizer, TokenIdentifier);

		GenericType* generic_type = (void*) new_type((Type) {
				.GenericType = {
					.compiler = (void*) &comp_GenericType,
					.flags = fConstExpr,
					.trace = identifier.trace,
					.index = base_generics.size,
				}
		});
		push(&declaration_setters, &generic_type->declaration);

		Wrapper* base_generic = (void*) new_type((Type) { .Wrapper = {
				.compiler = (void*) &comp_Wrapper,
				.flags = fConstExpr,
				.trace = identifier.trace,
				.anchor = (void*) new_type((Type) { .GenericType = {
						.compiler = (void*) &comp_GenericType,
						.flags = fConstExpr,
						.trace = identifier.trace,
						.index = base_generics.size,
				}}),
		}});
		push(&base_generics, (void*) base_generic);
		push(&declaration_setters, &base_generic->anchor->declaration);

		put(scope, identifier.trace.slice, (void*) new_node((Node) { .VariableDeclaration = {
					.compiler = (void*) &comp_VariableDeclaration,
					.trace = identifier.trace,
					.flags = fConst | fType,
					.type = (void*) generic_type,
					.const_value = (void*) generic_type,
		}}));

		if(!try(parser->tokenizer, ',', 0)) break;
	}
	expect(parser->tokenizer, '>');

	return (GenericsCollection) {
		.base_generics = base_generics,
		.scope = scope,
		.declaration_setters = declaration_setters,
	};
}

void lock_base_generics(Generics* generics) {
	generics->stack.size = 0;
}

void apply_generics(Declaration* declaration, GenericsCollection collection) {
	if(!collection.base_generics.size) return;

	push(&declaration->generics.stack,
			(declaration->generics.base = collection.base_generics));
	for(size_t i = 0; i < collection.declaration_setters.size; i++) {
		*collection.declaration_setters.data[i] = declaration;
	}

	init_scope(&declaration->generics.unique_map);
}

void append_generics_identifier(str* string, TypeList generics) {
	if(!generics.size) return;
	
	for(size_t i = 0; i < generics.size; i++) {
		strf(string, "__");
		stringify_type(generics.data[i], string, 1 << 0 /* StringifyAlphaNum */);
	}
}

strs filter_unique_generics_variants(TypeLists variants, str base) {
	strs identifiers = { 0 };
	Scope variants_set = { 0 };
	init_scope(&variants_set);

	for(size_t i = 0; i < variants.size; i++) {
		str identifier = strf(0, "%.*s", (int) base.size, base.data);
		append_generics_identifier(&identifier, variants.data[i]);

		if(get(variants_set, (Trace) { .slice = identifier })) {
			variants.data[i].size = 0;
		} else {
			put(&variants_set, identifier, 0);
		}

		push(&identifiers, identifier);
	}

	for(size_t i = 0; i < variants_set.cap; i++) {
		free(variants_set.data[i].data);
	}
	free(variants_set.data);

	return identifiers;
}
