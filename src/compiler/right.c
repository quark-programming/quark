#include "left.c"

void comp_struct_declaration(StructType* self, str* line, Compiler* compiler) {
	str typedef_line = strf(0, "struct ");
	self->parent->identifier->compiler((void*) self->parent->identifier,
			&typedef_line, compiler);

	strf(&typedef_line, " { ");
	for(size_t i = 0; i < self->fields.size; i++) {
		VariableDeclaration* var = self->fields.data[i];

		self->fields.data[i]->compiler((void*) self->fields.data[i], &typedef_line, compiler);
		strf(&typedef_line, "; ");
	}
	strf(&typedef_line, "};");

	push(&compiler->sections.data[0].lines, typedef_line);
	self->body->compiler((void*) self->body, line, compiler);
}

void comp_VariableDeclaration(VariableDeclaration* self, str* line, Compiler* compiler) {
	if(self->generics.base.size && !self->generics.stack.size) return;

	if(self->const_value && self->const_value->flags & fType) {
		const OpenedType opened = open_type((void*) self->const_value, 0);
		comp_struct_declaration((void*) opened.type, line, compiler);
		close_type(opened.actions, 0);
		return;
	}

	str decl_line;
	if(!self->is_inline) {
		decl_line = new_line(compiler);
		line = &decl_line;
	}

	self->type->compiler((void*) self->type, line, compiler);
	strf(line, " ");
	if(self->type->flags & fConst) strf(line, "const ");
	self->identifier->compiler((void*) self->identifier, line, compiler);

	if(!self->is_inline) {
		if(self->const_value) {
			strf(line, " = ");
			self->const_value->compiler(self->const_value, line, compiler);
		} else if(self->type->flags & fConst) {
			push(compiler->messages, Err(self->trace,
						str("expected declaration with '\33[35mconst\33[0m' type to have a "
							"value")));
		}

		strf(line, ";");
		push(&compiler->sections.data[compiler->open_section].lines, decl_line);
	}
}

void comp_BinaryOperation(BinaryOperation* self, str* line, Compiler* compiler) {
	strf(line, "(");
	self->left->compiler(self->left, line, compiler);
	strf(line, " %.*s ", (int) self->operator.size, self->operator.data);
	self->right->compiler(self->right, line, compiler);
	strf(line, ")");
}

void dual_function_compiler(FunctionDeclaration* self, Compiler* compiler, int hoisted) {
	const size_t previous_section = compiler->open_section;
	size_t section = compiler->open_section = compiler->sections.size;
	push(&compiler->sections, (CompilerSection) { 0 });

	str declaration_line = new_line(compiler);

	Type* const return_type = self->type->FunctionType.signature.data[0];
	return_type->compiler((void*) return_type, &declaration_line, compiler);

	strf(&declaration_line, " ");
	self->identifier->compiler((void*) self->identifier, &declaration_line, compiler);

	strf(&declaration_line, "(");
	for(size_t i = 0; i < self->arguments.size; i++) {
		if(i) strf(&declaration_line, ", ");
		VariableDeclaration* const argument = self->arguments.data[i];

		if(hoisted) {
			argument->type->compiler((void*) argument->type, &declaration_line, compiler);
		} else {
			argument->compiler((void*) argument, &declaration_line, compiler);
		}
	}
	strf(&declaration_line, hoisted ? ");" : ") {");
	push(&compiler->sections.data[hoisted ?: section].lines, declaration_line);

	if(hoisted) {
		compiler->open_section = previous_section;
		return;
	}

	self->body->compiler((void*) self->body, &declaration_line, compiler);

	str terminator_line = new_line(compiler);
	push(&compiler->sections.data[section].lines, strf(&terminator_line, "}"));
	compiler->open_section = previous_section;
}

void comp_FunctionDeclaration(FunctionDeclaration* self, str* line, Compiler* compiler) {
	if(self->flags & fExternal || self->generics.base.size && !self->generics.stack.size)
		return;

	dual_function_compiler(self, compiler, 1);
	dual_function_compiler(self, compiler, 0);
}

void function_type_typedef(FunctionType* self, Compiler* compiler, str identifier) {
	str typedef_line = strf(0, "typedef ");
	Type* const return_type = self->signature.data[0];
	return_type->compiler((void*) return_type, &typedef_line, compiler);

	strf(&typedef_line, "(*%.*s)(", (int) identifier.size, identifier.data);
	for(size_t i = 1; i < self->signature.size; i++) {
		if(i > 1) strf(&typedef_line, ", ");
		self->signature.data[i]->compiler(
				(void*) self->signature.data[i], &typedef_line, compiler);
	}
	strf(&typedef_line, ");");

	push(&compiler->sections.data[0].lines, typedef_line);
}

ssize_t global_function_typedef_id = 0;

void comp_FunctionType(FunctionType* self, str* line, Compiler* compiler) {
	if(!self->typedef_id) {
		const ssize_t typedef_id = ++global_function_typedef_id;
		self->typedef_id = -1;

		str base_identifier = strf(0, "__Function%zd", typedef_id);
		self->typedef_id = typedef_id;
	}

	if(self->typedef_id < 0) {
		strf(line, "/* circular */ void*");
		return;
	}

	strf(line, "__Function%zd", self->typedef_id);
	if(self->declaration->generics.stack.size) {
		append_generics_identifier(line, last(self->declaration->generics.stack));
	}
}

void comp_FunctionCall(FunctionCall* self, str* line, Compiler* compiler) {
	self->function->compiler(self->function, line, compiler);
	strf(line, "(");
	for(size_t i = 0; i < self->arguments.size; i++) {
		if(i) strf(line, ", ");
		self->arguments.data[i]->compiler(self->arguments.data[i], line, compiler);
	}
	strf(line, ")");
}

void comp_PointerType(PointerType* self, str* line, Compiler* compiler) {
	self->base->compiler((void*) self->base, line, compiler);
	strf(line, "*");
}
