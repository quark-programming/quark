#include "compiler.c"

void comp_NumericLiteral(NumericLiteral* self, str* line, Compiler* compiler) {
	strf(line, "%ld", self->number);
}

Compiler* generics_compiler_context;

void comp_Wrapper(Wrapper* self, str* line, Compiler* compiler) {
	generics_compiler_context = compiler;

	int applied_action;
	if(self->action.type) {
		applied_action = type_state_action(self->action, 0);
	}
	
	if(self->variable) {
		Node* const const_value = self->ref->Declaration.const_value;
		if(const_value && const_value->flags & fConstExpr) {
			if(!(self->flags & fType)) {
				strf(line, "(");
				self->type->compiler((void*) self->type, line, compiler);
				strf(line, ") ");
			}

			const_value->compiler(const_value, line, compiler);
		} else {
			self->ref->Declaration.identifier->compiler(
					(void*) self->ref->Declaration.identifier, line, compiler);
		}
	} else {
		if(!self->ref) {
			strf(line, self->flags & tfNumeric ? "int" : "/* auto */ int");
		} else {
			self->ref->compiler(self->ref, line, compiler);
		}
	}

	if(self->action.type && applied_action) {
		undo_type_state_action(self->action, 0);
	}
}

void comp_Identifier(Identifier* self, str* line, Compiler* compiler) {
	if(!(self->flags & fExternal)) {
		if(self->parent && self->parent->compiler == (void*) &comp_FunctionDeclaration
				&& !(self->declaration && self->declaration->compiler
					== (void*) &comp_VariableDeclaration)) {
			Identifier* const parent_ident = self->parent->FunctionDeclaration.identifier;
			parent_ident->compiler((void*) parent_ident, line, compiler);
			strf(line, "__");
		}

		if(self->parent && self->parent->compiler == (void*) &comp_StructType
				&& self->declaration->compiler != (void*) &comp_VariableDeclaration) {
			Identifier* const parent_ident = 
				((StructType*)(void*) self->parent)->parent->identifier;
			parent_ident->compiler((void*) parent_ident, line, compiler);
			strf(line, "__");
		}
	}

	strf(line, "%.*s", (int) self->base.size, self->base.data);

	if(self->declaration && self->declaration->generics.base.size
			&& self->declaration->generics.stack.size
			&& !(self->flags & fExternal)) {
		append_generics_identifier(line, last(self->declaration->generics.stack));
	}
}

void comp_Scope(Scope* self, str* line, Compiler* compiler) {
	if(self->wrap_brackets) {
		str bracket = new_line(compiler);
		push(&compiler->sections.data[compiler->open_section].lines, strf(&bracket, "{"));
	}

	CompilerSection* const section = compiler->sections.data + compiler->open_section;
	strf(&section->indent, "    ");

	for(size_t i = 0; i < self->declarations.size; i++) {
		if(self->declarations.data[i]->is_inline) continue;
		self->declarations.data[i]->compiler(
				(void*) self->declarations.data[i], line, compiler);
	}
	for(size_t i = 0; i < self->children.size; i++) {
		self->children.data[i]->compiler(self->children.data[i], line, compiler);
	}

	compiler->sections.data[compiler->open_section].indent.size -= 4;

	if(self->wrap_brackets) {
		str bracket = new_line(compiler);
		push(&compiler->sections.data[compiler->open_section].lines, strf(&bracket, "}"));
	}
}

void comp_Missing(Missing* self, str* line, Compiler* compiler) {
	push(compiler->messages, Err(self->trace,
				strf(0, "cannot find '\33[35m%.*s\33[0m' in scope",
					(int) self->trace.slice.size, self->trace.slice.data)));
}

void comp_External(External* self, str* line, Compiler* compiler) {
	strf(line, "%.*s", (int) self->data.size, self->data.data);
}

void comp_GenericType(GenericType* self, str* line, Compiler* compiler) {
	Type* const base = last(self->declaration->generics.stack).data[self->index];
	base->compiler((void*) base, line, compiler);
}

void comp_Prefix(Prefix* self, str* line, Compiler* compiler) {
	strf(line, "(%.*s", (int) self->prefix.size, self->prefix.data);
	self->child->compiler(self->child, line, compiler);
	strf(line, ")");
}
