#include "wrapper.h"

#include "parser/type/generics.h"

Vec(Action) extract_link_actions(Declaration** declaration, Vec(Action)* actions) {
    if(!actions) actions = &(Vec(Action)) { NULL };
    for(; (*declaration)->id == NodeDeclarationLink; *declaration = (*declaration)->DeclarationLink.link) {
        resv(actions, len((*declaration)->DeclarationLink.actions));
        for(u32 i = 0; i < len((*declaration)->DeclarationLink.actions); i++) {
            push(actions, (*declaration)->DeclarationLink.actions[i]);
        }
    }
    return *actions;
}

Wrapper* variable_of(Declaration* declaration, const Trace trace, unsigned long flags) {
    Vec(Action) actions = extract_link_actions(&declaration, NULL);
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

    if(actions) {
        assign_action((void*) variable, (Action) { ActionApplyCollection, .collection = actions }, true, true);
    }

    return variable;
}
