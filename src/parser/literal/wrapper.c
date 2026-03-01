#include "wrapper.h"

#include "parser/type/generics.h"

// TODO: handle generics here
Wrapper* variable_of(Declaration* declaration, const Trace trace, unsigned long flags) {
    while(declaration->id == NodeDeclarationLink) {
        declaration = declaration->DeclarationLink.link;
    }

    flags |= fConstExpr | fMutable | (declaration->flags & fType);

    Wrapper* variable = (void*) new_node((Node) {
        .Wrapper = {
            .id = WrapperVariable,
            .flags = flags,
            .trace = trace,
            .type = declaration->type,
            .Variable = { declaration },
        }
    });
    if(declaration->id == NodeFunctionDeclaration && declaration->FunctionDeclaration.actions) {
        assign_action((void*) variable, (Action) {
                          ActionApplyCollection,
                          .collection = declaration->FunctionDeclaration.actions
                      }, true, true);
    }

    return variable;
}
