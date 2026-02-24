#include "wrapper.h"

// TODO: handle generics here
Wrapper* variable_of(Declaration* declaration, const Trace trace, unsigned long flags) {
    while(declaration->id == NodeDeclarationLink) {
        declaration = declaration->DeclarationLink.link;
    }

    flags |= fConstExpr | fMutable | (declaration->flags & fType);

    return (void*) new_node((Node) {
        .Wrapper = {
            .id = WrapperVariable,
            .flags = flags,
            .trace = trace,
            .type = declaration->type,
            .Variable = { declaration },
        }
    });
}
