#include "function_call.h"

#include "../type/types.h"
#include "../statement/statement.h"
#include "righthand.h"
#include "../lefthand/reference.h"
#include "../type/clash_types.h"
#include "declaration/declaration.h"
#include "parser/literal/wrapper.h"
#include "parser/statement/scope.h"

Declaration* fetch_operator_override(Type* type, const String override) {
    const OpenedType open = open_type(type, 0);
    close_type(open.actions, 0);
    if(open.type->id != NodeStructType) return NULL;

    Scope* const overrides_scope = get(open.type->StructType.reference_structures, String("Operator"));
    if(!overrides_scope) return NULL;

    return find_in_scope_unwrapped(*overrides_scope, override);
}

Node* operator_override_n(Type* type, Node* self, NodeVector arguments, const String override, const Trace trace,
                          Parser* parser) {
    Declaration* const operator_override = fetch_operator_override(type, override);
    if(!operator_override) return NULL;

    Wrapper* override_variable = variable_of(operator_override, trace, 0);
    OpenedType const opened_lefthand = open_type(type, 0);

    override_variable->Variable.bound_self_argument = self;
    override_variable->type = make_type_standalone(override_variable->type);
    if(global_actions.size) override_variable->action = override_variable->type->Wrapper.action;

    close_type(opened_lefthand.actions, 0);
    return call_function((void*) override_variable, arguments, parser);
}

Node* operator_override(Type* type, Node* self, Node* argument, const String override, const Trace trace,
                        Parser* parser) {
    NodeVector arguments = { 0 };
    push(&arguments, argument);
    return operator_override_n(type, self, arguments, override, trace, parser);
}

Node* call_function(Node* function, NodeVector arguments, Parser* const parser) {
    const OpenedType opened_function_type = open_type(function->type, 0);
    FunctionType* const function_type = (void*) opened_function_type.type;

    // TODO: Operator::call override
    if(function_type->id != NodeFunctionType) {
        push(parser->tokenizer->messages, REPORT_ERR(function->trace, String("calling a non-function value")));
        close_type(opened_function_type.actions, 0);
        return function;
    }

    if(function->id == WrapperVariable && function->Wrapper.Variable.bound_self_argument) {
        // TODO: vector unshift() function macro
        resv(&arguments, 1);
        memmove(arguments.data + 1, arguments.data, arguments.size * sizeof(Node*));
        arguments.data[0] = function->Wrapper.Variable.bound_self_argument;
        arguments.size++;

        const OpenedType open_self = open_type(arguments.data[0]->type, 0);
        if(function_type->signature.size >= 2 && function_type->signature.data[1]->id == NodePointerType
           && open_self.type->id != NodePointerType) {
            arguments.data[0] = reference(arguments.data[0], arguments.data[0]->trace);
        }
        close_type(open_self.actions, 0);
    }

    for(size_t i = 0; i < arguments.size; i++) {
        if(i + 1 >= function_type->signature.size) {
            push(parser->tokenizer->messages,
                 REPORT_ERR(stretch(arguments.data[i]->trace, last(arguments)->trace),
                     String("too many arguments in function call")));
            push(parser->tokenizer->messages,
                 see_declaration((Declaration*) function_type->declaration, function->trace));
            break;
        }

        clash_types(function_type->signature.data[i + 1], arguments.data[i]->type, arguments.data[i]->trace,
                    parser->tokenizer->messages, 0);
    }

    if(arguments.size + 1 < function_type->signature.size) {
        push(parser->tokenizer->messages, REPORT_ERR(function->trace, String("not enough arguments in function call")));
        push(parser->tokenizer->messages, see_declaration((void*) function_type->declaration, function->trace));
    }

    Type* const return_type = make_type_standalone(function_type->signature.data[0]);
    close_type(opened_function_type.actions, 0);

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

    NodeVector arguments = collect_until(parser, &expression, ',', ')');

    return call_function(function, arguments, parser);
}
