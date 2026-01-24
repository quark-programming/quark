#include "wrapper.h"

#include "../../parser/type/types.h"
#include "../righthand/declaration/identifier.h"

void comp_Variable(void* void_self, String* line, Compiler* compiler) {
    Wrapper* const self = void_self;
    const bool applied_action = apply_action(self->action, 0);

    if(self->Variable.declaration->compilation_state != CompilationHoisted) {
        compile(self->Variable.declaration, line, compiler);
    }

    Node* const const_value = self->Variable.declaration->const_value;

    if(const_value && const_value->flags & fConstExpr) {
        if(!(self->flags & fType)) {
            strf(line, "((");
            compile(self->type, line, compiler);
            strf(line, ") ");
        }

        compile(const_value, line, compiler);

        if(!(self->flags & fType)) {
            strf(line, ")");
        }
    } else {
        String identifier = { 0 };
        resolve_identifier(self->Variable.declaration->identifier, &identifier);
        strf(line, "%.*s", FMT(identifier));
        free(identifier.data);
    }

    if(applied_action) remove_action(self->action, 0);
}

void comp_Auto(void* void_self, String* line, Compiler* compiler) {
    Wrapper* const self = void_self;
    // TODO: remove boolean result from apply_action
    const bool applied_action = apply_action(self->action, 0);

    if(!self->Auto.ref) {
        strf(line, self->flags & fNumeric ? "int" : "/* auto */ int");
    } else {
        compile(self->Auto.ref, line, compiler);
    }

    if(applied_action) remove_action(self->action, 0);
}

void comp_Surround(void* void_self, String* line, Compiler* compiler) {
    Wrapper* const self = void_self;
    const bool applied_action = apply_action(self->action, 0);

    strf(line, "%.*s", FMT(self->Surround.prefix));
    compile(self->Surround.child, line, compiler);
    strf(line, "%.*s", FMT(self->Surround.postfix));

    if(applied_action) remove_action(self->action, 0);
}
