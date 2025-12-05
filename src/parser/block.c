#include "../fs.c"
#include "right.c"

NodeList collect_until(Parser* parser, Node* (*supplier)(Parser*),
		unsigned char divider, unsigned char terminator) {
	NodeList collection = { 0 };

	while(parser->tokenizer->current.type && parser->tokenizer->current.type != terminator) {
		push(&collection, supplier(parser));
		if(divider && !try(parser->tokenizer, divider, 0)) break;
	}
	if(terminator) expect(parser->tokenizer, terminator);

	return collection;
}

Node* statement(Parser* parser) {
	if(parser->tokenizer->current.type == TokenIdentifier) {
		if(streq(parser->tokenizer->current.trace.slice, str("import"))) {
			Trace trace = next(parser->tokenizer).trace;
			str import_path = strf(0, ".");

			do {
				Trace section = expect(parser->tokenizer, TokenIdentifier).trace;
				strf(&import_path, "/%.*s", (int) section.slice.size, section.slice.data);
				trace = stretch(trace, section);
			} while(try(parser->tokenizer, TokenDoubleColon, NULL));
			expect(parser->tokenizer, ';');

			strf(&import_path, ".qk");
			push(&import_path, '\0');

			char* input_content = fs_readfile(import_path.data);
			if(!input_content) {
				push(parser->tokenizer->messages, Err(trace, strf(0,
								"unable to open read '%.*s'\n",
								(int) import_path.size, import_path.data)));
				return new_node((Node) { &comp_Ignore });
			}

			Tokenizer import_tokenizer = new_tokenizer(import_path.data, input_content,
					parser->tokenizer->messages);
			Tokenizer* const tokenizer = parser->tokenizer;
			parser->tokenizer = &import_tokenizer;

			Scope* scope = new_scope(NULL);
			scope->children = collect_until(parser, &statement, 0, 0);
			parser->tokenizer = tokenizer;
			return (void*) scope;
		}

		if(streq(parser->tokenizer->current.trace.slice, str("return"))) {
			Trace trace_start = next(parser->tokenizer).trace;
			Node* value = parser->tokenizer->current.type == ';' ? NULL : expression(parser);
			expect(parser->tokenizer, ';');

			if(last(parser->stack)->parent->compiler != (void*) &comp_FunctionDeclaration) {
				push(parser->tokenizer->messages, Err(trace_start,
							str("return statement needs to be "
								"inside of a function")));
			} else if(value) {
				clash_types(last(parser->stack)->parent->FunctionDeclaration.type
						->FunctionType.signature.data[0], value->type, value->trace,
						parser->tokenizer->messages, 0);
			}

			return new_node((Node) { .ReturnStatement = {
					.compiler = (void*) &comp_ReturnStatement,
					.value = value,
			}});
		}

		if(streq(parser->tokenizer->current.trace.slice, str("struct"))) {
			Trace trace_start = next(parser->tokenizer).trace;
			IdentifierInfo info = new_identifier(
					expect(parser->tokenizer, TokenIdentifier), parser);

			StructType* type = (void*) new_type((Type) {
					.StructType = {
						.compiler = (void*) &comp_StructType,
						.flags = fConstExpr,
						.trace = stretch(trace_start, info.trace),
					}
			});
			(type->body = info.generics_collection.scope ?: new_scope(NULL))->parent
				= (void*) type;
			// TODO: create a flag that only allows type to compile
			// if it is pointed to (in reference())
			// this will prevent circular types and allow structs
			// to reference themselves within themselves

			VariableDeclaration* declaration = (void*) new_node((Node) {
					.VariableDeclaration = {
						.compiler = (void*) &comp_VariableDeclaration,
						.flags = fConst | fType,
						.trace = type->trace,
						.type = (void*) type,
						.const_value = (void*) type,
						.identifier = info.identifier,
					}
			});
			put(info.scope, info.identifier->base, (void*) declaration);
			apply_generics((void*) declaration, info.generics_collection);
			info.identifier->declaration = (void*) declaration;
			type->parent = declaration;

			push(&parser->stack, type->body);
			expect(parser->tokenizer, '{');
	
			Node* next_declaration;
			while(parser->tokenizer->current.type && parser->tokenizer->current.type != '}'
					&& (next_declaration = statement(parser))->compiler == &comp_Ignore
					&& next_declaration->type) {
				if(next_declaration->type->compiler != (void*) &comp_Wrapper
						|| !next_declaration->type->Wrapper.variable
						|| !(next_declaration->type->flags & fIgnoreStatment)
						|| next_declaration->type->Wrapper.ref->compiler
						!= (void*) &comp_VariableDeclaration) break;
				next_declaration->type->Wrapper.ref->Declaration.is_inline = 1;
				push(&type->fields, (void*) next_declaration->type->Wrapper.ref);
			}

			NodeList declarations = { 0 };
			if(!try(parser->tokenizer, '}', NULL)) {
				push(&declarations, next_declaration);
				while(parser->tokenizer->current.type && !try(parser->tokenizer, '}', NULL)) {
					push(&declarations, statement(parser));
				}
			}

			parser->stack.size--;
			lock_base_generics(&declaration->generics);
			type->body->children = declarations;

			push(&last(parser->stack)->declarations, (void*) declaration);
			return new_node((Node) { .compiler = &comp_Ignore });
		}

		if(streq(parser->tokenizer->current.trace.slice, str("if")) ||
				streq(parser->tokenizer->current.trace.slice, str("while"))) {
			str keyword = next(parser->tokenizer).trace.slice;
			expect(parser->tokenizer, '(');

			NodeList inputs = { 0 };
			push(&inputs, expression(parser));
			expect(parser->tokenizer, ')');

			Node* body_node = statement(parser);
			Scope* body;

			if(body_node->compiler == (void*) &comp_Scope) {
				body = (void*) body_node;
			} else {
				body = new_scope(last(parser->stack)->parent);
				push(&body->children, body_node);
			}
			
			return new_node((Node) { .Control = {
					.compiler = (void*) &comp_Control,
					.keyword = keyword,
					.inputs = inputs,
					.body = body,
			}});
		}

		if(streq(parser->tokenizer->current.trace.slice, str("type"))) {
			next(parser->tokenizer);

			IdentifierInfo info = new_identifier(
					expect(parser->tokenizer, TokenIdentifier), parser);
			expect(parser->tokenizer, '=');
			Type* const type = (void*) expression(parser);
			type->flags |= fConstExpr;
			expect(parser->tokenizer, ';');

			put(info.scope, info.identifier->base, (void*) new_node(
						(Node) { .VariableDeclaration = {
						.compiler = (void*) &comp_VariableDeclaration,
						.trace = info.trace,
						.type = type,
						.flags = fType | fConst,
						.const_value = (void*) type,
						}}));
			return new_node((Node) { (void*) &comp_Ignore });
		}
	}

	if(try(parser->tokenizer, '{', NULL)) {
		Scope* block_scope = new_scope(last(parser->stack)->parent);
		push(&parser->stack, block_scope);
		block_scope->children = collect_until(parser, &statement, 0, '}');
		parser->stack.size--;
		block_scope->wrap_brackets = 1;
		return (void*) block_scope;
	}

	Node* expr = expression(parser);
	if(!(expr->flags & fStatementTerminated)) expect(parser->tokenizer, ';');

	if(expr->flags & fIgnoreStatment) return new_node((Node) {
			.compiler = &comp_Ignore,
			.type = (void*) expr,
	});

	return new_node((Node) { .Statement = {
			.compiler = (void*) &comp_Statement,
			.expression = expr,
	}});
}
