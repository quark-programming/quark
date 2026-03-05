#include "wrapper.h"

#include "parser/type/generics.h"

// TODO: handle generics here
Wrapper* variable_of(Declaration* declaration, const Trace trace, unsigned long flags) {
    Vec(Action) link_actions = NULL;

    while(declaration->id == NodeDeclarationLink) {
        if(declaration->DeclarationLink.actions) {
            push(&link_actions, { ActionApplyCollection, .collection = declaration->DeclarationLink.actions });
        }

        declaration = declaration->DeclarationLink.link;
    }

    flags |= fConstExpr | fMutable | (declaration->flags & fType);

    Wrapper* variable = (void*) new_node((Node) {
        .Wrapper = {
            .id = WrapperVariable,
            .flags = flags,
            .trace = trace,
            .type = declaration->type,
            // .action = { ActionApplyCollection * !!link_actions, .collection = link_actions },
            .Variable = { declaration },
        }
    });

    if(link_actions) {
        assign_action((void*) variable, (Action) { ActionApplyCollection, .collection = link_actions }, true, true);
    }

    // if(declaration->actions) {
    //     variable->action = (Action) { ActionApplyCollection, .collection = declaration->actions };
    //     // assign_action((void*) variable, (Action) {
    //     //                   ActionApplyCollection,
    //     //                   .collection = declaration->actions
    //     //               }, true, true);
    // }

    return variable;
}
