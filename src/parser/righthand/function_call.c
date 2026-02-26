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
    const OpenedType open = open_type(type, 0, 0);
    close_type(open.actions, 0, 0);
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
    OpenedType const opened_lefthand = open_type(type, 0, 0);

    override_variable->Variable.bound_self_argument = self;
    override_variable->type = make_type_standalone(override_variable->type);
    if(len(global_actions)) override_variable->action = override_variable->type->Wrapper.action;

    close_type(opened_lefthand.actions, 0, 0);
    return call_function((void*) override_variable, arguments, parser);
}

Node* operator_override(Type* type, Node* self, Node* argument, const str override, const Trace trace,
                        Parser* parser) {
    Vec(Node*) arguments = { 0 };
    push(&arguments, argument);
    return operator_override_n(type, self, arguments, override, trace, parser);
}

Node* call_function(Node* function, Vec(Node*) arguments, Parser* const parser) {
    const OpenedType opened_function_type = open_type(function->type, 0, 0);
    FunctionType* const function_type = (void*) opened_function_type.type;

    // TODO: Operator::call override
    if(function_type->id != NodeFunctionType) {
        push(parser->tokenizer->messages, MERROR(function->trace, str("calling a non-function value")));
        close_type(opened_function_type.actions, 0, 0);
        return function;
    }

    if(function->id == WrapperVariable && function->Wrapper.Variable.bound_self_argument) {
        // TODO: vector unshift() function macro
        resv(&arguments, 1);
        memmove(arguments + 1, arguments, len(arguments) * sizeof(Node*));
        arguments[0] = function->Wrapper.Variable.bound_self_argument;
        len(arguments)++;

        const OpenedType open_self = open_type(arguments[0]->type, 0, 0);
        if(len(function_type->signature) >= 2 && function_type->signature[1]->id == NodePointerType
           && open_self.type->id != NodePointerType) {
            arguments[0] = reference(arguments[0], arguments[0]->trace);
        }
        close_type(open_self.actions, 0, 0);
    }

    for(size_t i = 0; i < len(arguments); i++) {
        if(i + 1 >= len(function_type->signature)) {
            push(parser->tokenizer->messages,
                 MERROR(stretch(arguments[i]->trace, last(arguments)->trace),
                     str("too many arguments in function call")));
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

    Type* const return_type = make_type_standalone(function_type->signature[0]);
    close_type(opened_function_type.actions, 0, 0);

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
