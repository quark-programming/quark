#include "scope.h"
#include "../literal/wrapper.h"

Vec(Missing*) global_missing_identifiers = NULL;

Scope* new_scope(Declaration* const parent) {
    Scope* const scope = (void*) new_node((Node) {
        .Scope = {
            .id = NodeScope,
            .declaration = parent,
        }
    });

    if(!scope->declaration) scope->declaration = (void*) scope;
    return scope;
}

static bool declaration_found(Declaration** const possible_found) {
    return possible_found
           && !((*possible_found)->flags & fPrivate && (*possible_found)->identifier.parent_scope->flags & fPrivate);
}

Declaration* find_in_scope_unwrapped(const Scope scope, const str identifier) {
    Declaration** possible_found = get(scope.variables, identifier);
    Declaration* found = declaration_found(possible_found) ? *possible_found : NULL;

    for(u32 i = 0; !found && i < len(scope.wildcards); i++) {
        found = find_in_scope_unwrapped(*scope.wildcards[i], identifier);
    }

    return found;
}

Wrapper* find_in_scope(const Scope scope, const Trace identifier) {
    Declaration* declaration = find_in_scope_unwrapped(scope, identifier.source);
    return declaration ? variable_of(declaration, identifier, 0) : NULL;
}

Declaration* find_on_stack_unwrapped(Stack const stack, const str identifier) {
    for(size_t i = len(stack); i > 0; i--) {
        Declaration* possible_found = find_in_scope_unwrapped(*stack[i - 1], identifier);
        if(possible_found) return possible_found;
    }
    return NULL;
}

Wrapper* find_on_stack(Stack const stack, const Trace identifier) {
    Declaration* declaration = find_on_stack_unwrapped(stack, identifier.source);
    return declaration ? variable_of(declaration, identifier, 0) : NULL;
}
