#include "left.c"

NodeList collect_until(Parser* parser, Node* (*supplier)(Parser*),
                       unsigned char divider, unsigned char terminator);
Node* right(Node* lefthand, Parser* parser, unsigned char precedence);
Node* statement(Parser* parser);

enum {
    RightBinary,
    RightAltBinary,
    RightDeclaration,
    RightAssignment,
    RightIncDec,
    RightCall,
    RightFieldAccess,
    RightCompare,
    RightIndex,
    RightOptional,
};

typedef struct {
    unsigned char precedence : 4,
        type : 4;
} RightOperator;

extern int collecting_type_arguments;
// https://en.cppreference.com/w/c/language/operator_precedence.html
RightOperator right_operator_table[128] = {
    [TokenDoublePlus] = {1, RightIncDec}, [TokenDoubleMinus] = {1, RightIncDec},
    ['['] = {1, RightIndex}, ['('] = {1, RightCall},
    ['.'] = {1, RightFieldAccess}, [TokenRightArrow] = {1, RightFieldAccess},
    ['?'] = {1, RightOptional},

    ['*'] = {3, RightAltBinary}, ['/'] = {3}, ['%'] = {3},

    ['+'] = {4}, ['-'] = {4},

    [TokenDoubleLess] = {5}, [TokenDoubleGreater] = {5},

    ['<'] = {6, RightCompare}, ['>'] = {6, RightCompare},
    [TokenLessEqual] = {6, RightCompare}, [TokenGreaterEqual] = {6, RightCompare},

    [TokenDoubleEqual] = {7, RightCompare}, [TokenNotEqual] = {7, RightCompare},

    ['&'] = {8}, ['^'] = {9}, ['|'] = {10},

    [TokenDoubleAnd] = {11, RightCompare}, [TokenDoubleOr] = {12, RightCompare},

    [TokenIdentifier] = {13, RightDeclaration},

    ['='] = {14, RightAssignment}, [TokenPlusEqual] = {14, RightAssignment},
    [TokenMinusEqual] = {14, RightAssignment}, [TokenTimesEqual] = {14, RightAssignment},
    [TokenDivideEqual] = {14, RightAssignment}, [TokenModEqual] = {14, RightAssignment},
    [TokenAndEqual] = {14, RightAssignment}, [TokenXorEqual] = {14, RightAssignment},
    [TokenOrEqual] = {14, RightAssignment},
};

Node* expression(Parser* parser) {
    return right(left(parser), parser, 15);
}

int filter_missing(Type* type, void* ignore) {
    (void)ignore; // TODO: remove or use
    return type->compiler == (void*)&comp_Missing;
}

int recycle_missing(Type* missing, Type* _, Parser* parser) {
    (void)_; // TODO: remove or use
    if (missing->compiler != (void*)&comp_Missing) return 0;
    Wrapper* possible_found = find(parser->stack, missing->trace);

    if (possible_found && possible_found->flags & fType)
    {
        *missing = *(Type*)(void*)possible_found;
        unbox((void*)possible_found);
        return 1;
    }

    unbox((void*)possible_found);
    return 0;
}

Wrapper* declaration(Node* type, Token identifier, Parser* parser) {
    if (!(type->flags & fType))
    {
        push(parser->tokenizer->messages, REPORT_ERR(type->trace,
                 str("expected a type before declaration identifier")));
        push(parser->tokenizer->messages, REPORT_HINT(str("also try '\33[35mtypeof(expr)\33[0m'")));
        type = (void*)type->type;
    }

    IdentifierInfo info = new_identifier(identifier, parser);

    if (try(parser->tokenizer, '(', 0))
    {
        Trace trace_start = stretch(type->trace, info.trace);

        FunctionType* function_type = (void*)new_type((Type){
            .FunctionType = {
                .compiler = (void*)&comp_FunctionType,
                .trace = trace_start,
            }
        });
        push(&function_type->signature, (void*) type);

        FunctionDeclaration* declaration = (void*)new_node((Node){
            .FunctionDeclaration = {
                .compiler = (void*)&comp_FunctionDeclaration,
                .flags = info.identifier->flags & fExternal,
                .trace = trace_start,
                .type = (void*)function_type,
                .identifier = info.identifier,
            }
        });
        declaration->body = info.generics_collection.scope ? : new_scope(NULL);
        declaration->body->parent = (void*)declaration;

        function_type->declaration = declaration;
        info.identifier->declaration = (void*)declaration;

        apply_generics((void*)declaration, info.generics_collection);
        push(&parser->stack, declaration->body);
        traverse_type((void*)type, NULL, (void*)&recycle_missing, parser, TraverseGenerics);

        push(&info.scope->declarations, (void*) declaration);
        put(info.scope, info.identifier->base, (void*)declaration);

        NodeList argument_declarations = collect_until(parser, &expression, ',', ')');

        for (size_t i = 0; i < argument_declarations.size; i++)
        {
            Node* const argument = argument_declarations.data[i];
            if (argument->compiler == (void*)&comp_Wrapper && argument->Wrapper.variable
                && argument_declarations.data[i]->flags & fIgnoreStatment)
            {
                push(&function_type->signature, argument->type);
                VariableDeclaration* const arg_declaration = (void*)argument->Wrapper.ref;
                arg_declaration->is_inline = 1;
                push(&declaration->arguments, arg_declaration);
            }
            else
            {
                push(parser->tokenizer->messages, REPORT_ERR(
                         argument_declarations.data[i]->trace,
                         str("expected an argument declaration")));
            }
        }
        free(argument_declarations.data);

        if (!(declaration->flags & fExternal))
        {
            expect(parser->tokenizer, '{');
            declaration->body->children = collect_until(parser, &statement, 0, '}');
        }
        lock_base_generics(&declaration->generics);

        parser->stack.size--;
        return variable_of((void*)declaration, declaration->trace,
                           fIgnoreStatment | fStatementTerminated * !(declaration->flags & fExternal));
    }

    VariableDeclaration* declaration = (void*)new_node((Node){
        .VariableDeclaration = {
            .compiler = (void*)&comp_VariableDeclaration,
            .trace = stretch(type->trace, info.trace),
            .type = (void*)type,
            .identifier = info.identifier,
        }
    });
    info.identifier->declaration = (void*)declaration;

    push(&info.scope->declarations, (void*) declaration);
    put(info.scope, info.identifier->base, (void*)declaration);

    if (type->flags & fConst && try(parser->tokenizer, '=', 0))
    {
        declaration->const_value = right(left(parser), parser, 14);
        clash_types(declaration->type, declaration->const_value->type,
                    declaration->trace, parser->tokenizer->messages, 0);
    }

    return variable_of((void*)declaration, declaration->trace, fIgnoreStatment);
}

Message see_declaration(Declaration* declaration, Node* node) {
    if (!declaration) {
        return REPORT_INFO((Trace){0}, str("declaration not found"));
    }
    return REPORT_INFO(declaration->trace, strf(0, "declaration of '\33[35m%.*s\33[0m'", (int)node->trace.slice.size, node->trace.slice.data));
}

Node* temp_value(Node* value, Parser* parser, unsigned* set_id) {
    static unsigned id = 0;
    if (set_id) *set_id = id;
    const str declaration_code = strf(0, "auto __qv%u = extern<auto> \"\";%c", id++, '\0');
    Node* declaration = eval_w(parser->tokenizer->current.trace.filename,
                               declaration_code.data, parser, &statement);
    declaration->Statement.expression->BinaryOperation.right = value;
    clash_types(declaration->Statement.expression->BinaryOperation.left->type,
                value->type, value->trace, parser->tokenizer->messages, 0);
    return declaration;
}

Node* right(Node* lefthand, Parser* parser, unsigned char precedence) {
    RightOperator operator;
outer_while:
    while ((operator = right_operator_table[parser->tokenizer->current.type]).precedence)
    {
        if (operator.precedence >= precedence + (operator.type == RightAssignment)) break;
        if (operator.precedence == 6 && collecting_type_arguments) break;

        switch (operator.type)
        {
        case RightOptional:
            {
                Trace trace_end = next(parser->tokenizer).trace;

                if ((operator = right_operator_table[parser->tokenizer->current.type])
                    .precedence <= 1)
                {
                    unsigned temp_id;
                    Node* temp = temp_value(lefthand, parser, &temp_id);

                    const str left_value_code = strf(0, "__qv%u.value%c", temp_id, '\0');
                    Node* some_branch = right(eval(NULL, left_value_code.data, parser),
                                              parser, 2);

                    Scope* collection = new_scope(NULL);
                    collection->trace = lefthand->trace;
                    collection->wrap_brackets = 1;
                    push(&collection->children, temp);

                    const OpenedType open = open_type(some_branch->type, 0);
                    if (open.type->compiler == (void*)comp_External &&
                        streq(open.type->External.data, str("void")))
                    {
                        Node* if_statement = eval_w(NULL, strf(0,
                                                               "if(__qv%u.some) extern<auto> _;%c", temp_id, '\0').data,
                                                    parser, &statement);
                        if_statement->Control.body->children.data[0]->Statement.expression
                            = some_branch;
                        push(&collection->children, if_statement);
                        close_type(open.actions, 0);
                        return (void*)collection;
                    }
                    close_type(open.actions, 0);

                    unsigned value_id;
                    Node* value = temp_value(eval(NULL, "Option::None()", parser),
                                             parser, &value_id);

                    const str if_statement_code = strf(0,
                                                       "if(__qv%u.some) __qv%u = Option::Some(extern<auto> _);%c",
                                                       temp_id, value_id, '\0');
                    Node* if_statement = eval_w(NULL, if_statement_code.data, parser,
                                                &statement);
                    printf("%p %p\n",
                           if_statement->Control.body->children.data[0]->compiler,
                           comp_Statement);
                    // exit(1);
                    clash_types(if_statement->Control.body->children.data[0]->Statement
                                                                            .expression->BinaryOperation.right->
                                                                            FunctionCall.arguments
                                                                            .data[0]->type, some_branch->type,
                                some_branch->trace,
                                parser->tokenizer->messages, 0);
                    if_statement->Control.body->children.data[0]->Statement.expression
                                                                ->BinaryOperation.right->FunctionCall.arguments.data[0]
                        = some_branch;

                    push(&collection->children, value);
                    push(&collection->children, if_statement);
                    collection->value = eval(NULL, strf(0, "__qv%u%c", value_id, 0).data,
                                             parser);
                    collection->type = collection->value->type;

                    lefthand = (void*)collection;
                    break;
                }

                if (lefthand->flags & fType)
                {
                    Wrapper* option = (void*)eval("option", "Option", parser);
                    clash_types(option->action.TypeList.data[0], (void*)lefthand,
                                stretch(lefthand->trace, trace_end),
                                parser->tokenizer->messages, 0);
                    lefthand = (void*)option;
                    break;
                }
                break;
            }

        case RightIncDec:
            {
                if (!(lefthand->flags & fMutable) || lefthand->type->flags & fConst)
                {
                    push(parser->tokenizer->messages, REPORT_ERR( lefthand->trace,
                             str("left hand of assignment is not a mutable value")));
                }

                Trace operator = next(parser->tokenizer).trace;

                lefthand = new_node((Node){
                    .Postfix = {
                        .compiler = (void*)&comp_Postfix,
                        .trace = operator,
                        .type = lefthand->type,
                        .child = lefthand,
                        .postfix = operator.slice,
                    }
                });
                break;
            }

        case RightAltBinary:
            {
                switch (parser->tokenizer->current.type)
                {
                case '*':
                    if (!(lefthand->flags & fType)) break;
                    lefthand = reference(lefthand, stretch(lefthand->trace,
                                                           next(parser->tokenizer).trace));
                    goto outer_while;
                }
                goto binary;
            }

        case RightAssignment:
            if (!(lefthand->flags & fMutable) || lefthand->type->flags & fConst)
            {
                push(parser->tokenizer->messages, REPORT_ERR(lefthand->trace,
                         str("left hand of assignment is not a mutable value")));
            }
        case RightCompare:
        case RightBinary: binary:
            {
                Token operator_token = next(parser->tokenizer);
                Node* righthand = right(left(parser), parser, operator.precedence);

                clash_types(lefthand->type, righthand->type,
                            stretch(lefthand->trace, righthand->trace),
                            parser->tokenizer->messages, 0);

                lefthand = new_node((Node){
                    .BinaryOperation = {
                        .compiler = (void*)&comp_BinaryOperation,
                        .flags = lefthand->flags & righthand->flags,
                        .trace = stretch(lefthand->trace, righthand->trace),
                        .type = lefthand->type,
                        .left = lefthand,
                        .operator = operator_token.trace.slice,
                        .right = righthand,
                    }
                });
                break;
            }

        case RightDeclaration:
            {
                if (!(lefthand->flags & fType)) return lefthand;
                lefthand = (void*)declaration(lefthand, next(parser->tokenizer), parser);
                break;
            }

        case RightIndex:
            {
                next(parser->tokenizer);
                Node* index = expression(parser);

                lefthand = dereference(new_node((Node){
                                           .BinaryOperation = {
                                               .compiler = (void*)&comp_BinaryOperation,
                                               .type = lefthand->type,
                                               .left = lefthand,
                                               .operator = str("+"),
                                               .right = index,
                                           }
                                       }), stretch(lefthand->trace, expect(parser->tokenizer, ']').trace),
                                       parser->tokenizer->messages);
                break;
            }

        case RightCall:
            {
                next(parser->tokenizer);

                TypeList signature = {0};
                Type* return_type;

                const OpenedType opened_function_type = open_type(lefthand->type, 0);
                Type* const open_function_type = opened_function_type.type;

                if (open_function_type->compiler != (void*)&comp_FunctionType)
                {
                    push(parser->tokenizer->messages, REPORT_ERR(lefthand->trace,
                             str("calling a non-function value")));

                    return_type = new_type((Type){
                        .Wrapper = {
                            .compiler = (void*)&comp_Wrapper,
                            .trace = lefthand->trace,
                        }
                    });
                }
                else
                {
                    signature = open_function_type->FunctionType.signature;
                    return_type = signature.data[0];
                }

                NodeList arguments = collect_until(parser, &expression, ',', ')');
                if (lefthand->compiler == (void*)&comp_Wrapper
                    && lefthand->Wrapper.bound_self)
                {
                    resv(&arguments, 1);
                    memmove(arguments.data + 1, arguments.data,
                            arguments.size * sizeof(Node*));
                    arguments.data[0] = lefthand->Wrapper.bound_self;
                    arguments.size++;

                    const OpenedType open_self = open_type(arguments.data[0]->type, 0);
                    if (signature.data[1]->compiler == (void*)&comp_PointerType
                        && open_self.type->compiler != (void*)&comp_PointerType)
                    {
                        arguments.data[0] = reference(arguments.data[0],
                                                      arguments.data[0]->trace);
                    }
                    close_type(open_self.actions, 0);
                }

                for (size_t i = 0; i < arguments.size; i++)
                {
                    if (i + 1 >= signature.size)
                    {
                        push(parser->tokenizer->messages, REPORT_ERR(
                                 stretch(arguments.data[i]->trace, last(arguments)->trace),
                                 str("too many arguments in function call")));
                        push(parser->tokenizer->messages,
                             see_declaration((Declaration*)open_function_type->FunctionType.declaration, lefthand));
                        break;
                    }

                    clash_types(signature.data[i + 1], arguments.data[i]->type,
                                arguments.data[i]->trace, parser->tokenizer->messages, 0);
                }

                if (arguments.size + 1 < signature.size)
                {
                    push(parser->tokenizer->messages, REPORT_ERR(lefthand->trace,
                             str("not enough arguments in function call")));
                    push(parser->tokenizer->messages,
                         see_declaration(
                             (void*) open_function_type->FunctionType.declaration,
                             lefthand));
                }

                return_type = make_type_standalone(return_type);
                close_type(opened_function_type.actions, 0);

                lefthand = new_node((Node){
                    .FunctionCall = {
                        .compiler = (void*)&comp_FunctionCall,
                        .type = return_type,
                        .trace = lefthand->trace,
                        .function = lefthand,
                        .arguments = arguments,
                    }
                });
                break;
            }

        case RightFieldAccess:
            {
                Type* type = lefthand->type;
                if (parser->tokenizer->current.type == TokenRightArrow)
                {
                    type = (void*)dereference((void*)type, lefthand->trace,
                                              parser->tokenizer->messages);
                }

                const OpenedType opened = open_type((void*) type, 0);
                StructType* const struct_type = (void*)opened.type;

                str operator_token = next(parser->tokenizer).trace.slice;
                Token field_token = expect(parser->tokenizer, TokenIdentifier);

                // TODO: error message if not struct
                if (struct_type->compiler != (void*)&comp_StructType)
                {
                    push(parser->tokenizer->messages, REPORT_ERR(lefthand->trace, strf(0,
                             "'\33[35m%.*s\33[0m' is not a structure",
                             (int) lefthand->trace.slice.size,
                             lefthand->trace.slice.data)));
                    close_type(opened.actions, 0);
                    return lefthand;
                }

                ssize_t found_index = -1;
                for (size_t i = 0; i < struct_type->fields.size; i++)
                {
                    if (streq(field_token.trace.slice,
                              struct_type->fields.data[i]->identifier->base))
                    {
                        found_index = i;
                    }
                }

                if (found_index < 0)
                {
                    Wrapper* child = get(*struct_type->body, field_token.trace);
                    if (child)
                    {
                        child->bound_self = lefthand;
                        lefthand->type = make_type_standalone(lefthand->type);
                        child->action = lefthand->type->Wrapper.action;
                        child->type = make_type_standalone(child->type);

                        close_type(opened.actions, 0);
                        lefthand = (void*)child;
                        break;
                    }

                    push(parser->tokenizer->messages, REPORT_ERR(
                             field_token.trace, strf(0,
                                 "no field named '\33[35m%.*s\33[0m' on struct "
                                 "'\33[35m%.*s\33[0m'",
                                 (int) field_token.trace.slice.size,
                                 field_token.trace.slice.data,
                                 (int) lefthand->trace.slice.size,
                                 lefthand->trace.slice.data)));
                    push(parser->tokenizer->messages, see_declaration((void*) struct_type,
                             lefthand));
                    break;
                }

                Type* field_type = make_type_standalone(
                    struct_type->fields.data[found_index]->type);
                close_type(opened.actions, 0);

                lefthand = new_node((Node){
                    .BinaryOperation = {
                        .compiler = (void*)&comp_BinaryOperation,
                        .flags = fMutable | (lefthand->flags & fConstExpr),
                        .trace = stretch(lefthand->trace, field_token.trace),
                        .type = field_type,
                        .left = lefthand,
                        .operator = operator_token,
                        .right = new_node((Node){
                            .External = {
                                .compiler = (void*)&comp_External,
                                .data = field_token.trace.slice,
                            }
                        }),
                    }
                });
                break;
            }
        }
    }

    return lefthand;
}
