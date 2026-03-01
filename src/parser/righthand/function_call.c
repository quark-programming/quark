#include "function_call.h"

#include "../type/types.h"
#include "../statement/statement.h"
#include "righthand.h"
#include "../lefthand/reference.h"
#include "../type/clash_types.h"
#include "declaration/declaration.h"
#include "parser/literal/wrapper.h"
#include "parser/statement/scope.h"

Declaration* fetch_operator_override(Type* type, const str override) {
    const OpenedType open = open_type(type, 0, 2);
    close_type(open.actions, 0, 2);
    if(open.type->id != NodeStructType) return NULL;

    Scope* const overrides_scope = get(open.type->StructType.traits, str("Operator"));
    if(!overrides_scope) return NULL;

    return find_in_scope_unwrapped(*overrides_scope, override);
}

Node* operator_override_n(Type* type, Node* self, Vec(Node*) arguments, const str override, const Trace trace,
                          Parser* parser) {
    Declaration* const operator_override = fetch_operator_override(type, override);
    if(!operator_override) return NULL;

    Wrapper* override_variable = variable_of(operator_override, trace, 0);
    OpenedType const opened_lefthand = open_type(type, 0, 2);

    override_variable->Variable.bound_self_argument = self;
    override_variable->type = make_type_standalone(override_variable->type, 2);
    if(len(global_actions[2])) override_variable->action = override_variable->type->Wrapper.action;

    close_type(opened_lefthand.actions, 0, 2);
    return call_function((void*) override_variable, arguments, parser);
}

Node* operator_override(Type* type, Node* self, Node* argument, const str override, const Trace trace,
                        Parser* parser) {
    Vec(Node*) arguments = { 0 };
    push(&arguments, argument);
    return operator_override_n(type, self, arguments, override, trace, parser);
}

Node* call_function(Node* function, Vec(Node*) arguments, Parser* const parser) {
    const OpenedType opened_function_type = open_type(function->type, 0, 2);
    FunctionType* const function_type = (void*) opened_function_type.type;

    // TODO: Operator::call override
    if(function_type->id != NodeFunctionType) {
        push(parser->tokenizer->messages, MERROR(function->trace, str("calling a non-function value")));
        close_type(opened_function_type.actions, 0, 2);
        return function;
    }

    Node* const bound_self = function->id == WrapperVariable
                                 ? function->Wrapper.Variable.bound_self_argument
                                 : function->id == NodeTraitAccess
                                       ? function->TraitAccess.bound_self_argument
                                       : NULL;
    if(bound_self) {
        // TODO: vector unshift() function macro
        resv(&arguments, 1);
        memmove(arguments + 1, arguments, len(arguments) * sizeof(Node*));
        arguments[0] = bound_self;
        len(arguments)++;

        const OpenedType open_self = open_type(arguments[0]->type, 0, 2);
        const OpenedType open_argument = open_type(len(function_type->signature) >= 2
                                                   ? function_type->signature[1] : NULL, 0, 2);

        if(open_argument.type) {
            if(open_self.type->id != NodePointerType && function_type->signature[1]->id == NodePointerType) {
                arguments[0] = reference(arguments[0], arguments[0]->trace);
            } else if(open_self.type->id == NodePointerType && function_type->signature[1]->id != NodePointerType) {
                arguments[0] = dereference(arguments[0], arguments[0]->trace, parser->tokenizer->messages);
            }
        }

        close_type(open_argument.actions, 0, 2);
        close_type(open_self.actions, 0, 2);
    }

    for(size_t i = 0; i < len(arguments); i++) {
        if(i + 1 >= len(function_type->signature)) {
            push(parser->tokenizer->messages,
                 MERROR(stretch(arguments[i]->trace, last(arguments)->trace),
                     str("excess arguments in function call")));
            push(parser->tokenizer->messages,
                 see_declaration((Declaration*) function_type->declaration, function->trace));
            break;
        }

        clash_types(function_type->signature[i + 1], arguments[i]->type, arguments[i]->trace,
                    parser->tokenizer->messages, 0);
    }

    if(len(arguments) + 1 < len(function_type->signature)) {
        push(parser->tokenizer->messages, MERROR(function->trace, str("not enough arguments in function call")));
        push(parser->tokenizer->messages, see_declaration((void*) function_type->declaration, function->trace));
    }

    Type* const return_type = make_type_standalone(function_type->signature[0], 2);
    close_type(opened_function_type.actions, 0, 2);

    return new_node((Node) {
        .FunctionCall = {
            .id = NodeFunctionCall,
            .type = return_type,
            .trace = function->trace,
            .function = function,
            .arguments = arguments,
        }
    });
}

Node* parse_function_call(Node* function, Parser* parser) {
    next(parser->tokenizer);
    Vec(Node*) arguments = collect_until(parser, &expression, ',', ')');
    return call_function(function, arguments, parser);
}
