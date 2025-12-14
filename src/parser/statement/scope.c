#include "scope.h"
#include "../literal/wrapper.h"

Scope* new_scope(Declaration* const parent) {
    Scope* const scope = (void*) new_node((Node) {
        .Scope = {
            .id = NodeScope,
            .parent = parent,
        }
    });

    if(!scope->parent) scope->parent = (void*) scope;

    return scope;
}

Wrapper* find_in_scope(const Scope scope, const Trace identifier) {
    Declaration** possible_found = get(scope.variables, identifier.source);
    return possible_found ? variable_of(*possible_found, identifier, 0) : NULL;
}

Wrapper* find_on_stack(const Stack stack, const Trace identifier) {
    for(size_t i = stack.size; i > 0; i--) {
        Wrapper* possible_found = find_in_scope(*stack.data[i - 1], identifier);
        if(possible_found) return possible_found;
    }
    return NULL;
}