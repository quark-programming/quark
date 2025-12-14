#include "wrapper.h"

Wrapper* variable_of(Declaration* declaration, const Trace trace, unsigned long flags) {
    declaration->observerd = true;
    flags |= fConstExpr | fMutable | (declaration->flags & fType);

    return (void*) new_node((Node) {
        .Wrapper = {
            .id = NodeWrapper,
            .flags = flags,
            .trace = trace,
            .type = declaration->type,
            .Variable = { declaration },
        }
    });
}
