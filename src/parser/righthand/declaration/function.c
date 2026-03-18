#include "function.h"

#include "clargs.h"
#include "identifier.h"
#include "tty.h"
#include "../../statement/scope.h"
#include "../../type/types.h"
#include "../../type/generics.h"
#include "../../statement/statement.h"
#include "../righthand.h"
#include "../../literal/wrapper.h"
#include "parser/lefthand/lefthand.h"
#include "parser/lefthand/reference.h"
#include "parser/type/clash_types.h"
#include "parser/type/traverse_type.h"

typedef struct RecycleAccumulator {
    Vec(Message)* messages;
    Scope* scope;
} RecycleAccumulator;

static int recycle_missing_generics(Type* missing, Type* ignore, void* void_accumulator) {
    (void) ignore;
    RecycleAccumulator* const accumulator = void_accumulator;

    if(missing->id != WrapperAuto || !missing->Wrapper.Auto.missing) return 0;
    Wrapper* possible_found = find_in_scope(*accumulator->scope, missing->trace);

    if(possible_found && possible_found->flags & fType) {
        // missing->Wrapper = *possible_found;
        clash_types(missing, (void*) possible_found, missing->trace, accumulator->messages, 0);
        missing->Wrapper.Auto.missing = false;
        return 1;
    }

    unbox((void*) possible_found);
    return 0;
}

static Argument create_self_literal(const Trace trace, Declaration* declaration, Declaration* func_declaration,
                                    Parser* parser, bool is_ref) {
    Type* type = NULL;

    if(declaration->id != NodeStructDeclaration) {
        push(parser->tokenizer->messages,
             MERROR(trace, str("Cannot create self literal outside of a struct declaration")));
        type = new_type((Type) { .Wrapper = { WrapperAuto, 0, trace } });
    } else if(declaration->type->StructType.is_trait) {
        type = create_generic(trace, NULL);
        type->Wrapper.Auto.required_traits = vec(variable_of(func_declaration, trace, 0));
        type->Wrapper.Auto.non_matching_required_traits = vec(variable_of(declaration, trace, 0));

        apply_type_arguments(type->Wrapper.Auto.required_traits[0], NULL, true);
        apply_type_arguments(type->Wrapper.Auto.non_matching_required_traits[0], NULL, true);

        type->Wrapper.Auto.priority = -2;

        push(&func_declaration->generics.base_type_arguments, type);
        if(len(func_declaration->generics.base_type_arguments) == 1) {
            push(&func_declaration->generics.type_arguments_stacks[2], func_declaration->generics.base_type_arguments);
        } else {
            func_declaration->generics.type_arguments_stacks[2][0] = func_declaration->generics.base_type_arguments;
        }
    } else {
        Wrapper* wrapper = variable_of(declaration, trace, 0);
        apply_type_arguments(wrapper, parser, false);
        type = (void*) wrapper;
    }

    const Argument argument = {
        .type = is_ref ? (void*) reference((void*) type, trace) : type,
        .identifier = str("self"),
    };

    return argument;
}

static void parse_function_arguments(FunctionType* function_type, FunctionDeclaration* declaration, Parser* parser,
                                     bool allow_unnamed) {
    bool unnamed = false;

    while(parser->tokenizer->current.type && parser->tokenizer->current.type != ')') {
        Argument argument = { 0 };

        if(parser->tokenizer->current.type == '&') {
            const Token snapshot = next(parser->tokenizer);
            if(parser->tokenizer->current.type == TokenIdentifier
               && streq(parser->tokenizer->current.trace.source, str("self"))) {
                argument = create_self_literal(stretch(snapshot.trace, next(parser->tokenizer).trace),
                                               (void*) declaration->identifier.parent_scope->declaration,
                                               (void*) function_type->declaration, parser, true);
            } else {
                parser->tokenizer->current = snapshot;
            }
        } else if(parser->tokenizer->current.type == TokenIdentifier
                  && streq(parser->tokenizer->current.trace.source, str("self"))) {
            argument = create_self_literal(next(parser->tokenizer).trace,
                                           (void*) declaration->identifier.parent_scope->declaration,
                                           (void*) function_type->declaration, parser, false);
        }

        if(!argument.type) {
            argument.type = (void*) righthand_expression(lefthand_expression(parser), parser, 13);

            if(allow_unnamed && parser->tokenizer->current.type != TokenIdentifier) {
                unnamed = true;
            } else if(!unnamed) {
                argument.identifier = expect(parser->tokenizer, TokenIdentifier).trace.source;
                allow_unnamed = false;
            }
        }

        if(!(argument.type->flags & fType)) {
            push(parser->tokenizer->messages,
                 MERROR(argument.type->trace, strf(0, iftty(HERR"%.*s"H" is not a type", "%.*s is not a type"),
                     fmtof(argument.type->trace.source))));
        }

        if(argument.identifier.len) {
            Declaration* const argument_declaration = (void*) new_node((Node) {
                .VariableDeclaration = {
                    .id = NodeVariableDeclaration,
                    .type = argument.type,
                    .compilation_state = CompilationLocal,
                    .identifier = {
                        .base = argument.identifier,
                        .parent_scope = (void*) parser->stack[0],
                    },
                }
            });
            argument_declaration->identifier.parent_declaration = argument_declaration;
            put(&declaration->body->variables, argument.identifier, argument_declaration);
        }

        push(&declaration->arguments, argument);
        push(&function_type->signature, argument.type);

        if(!try(parser->tokenizer, ',', NULL)) break;
    }
    expect(parser->tokenizer, ')');
}

Node* parse_function_declaration(Type* return_type, IdentifierInfo info, Parser* parser, const bool no_body) {
    const Trace trace_start = stretch(return_type->trace, info.trace);

    FunctionType* function_type = (void*) new_type((Type) {
        .FunctionType = {
            .id = NodeFunctionType,
            .trace = trace_start,
        }
    });
    push(&function_type->signature, return_type);

    FunctionDeclaration* declaration = (void*) new_node((Node) {
        .FunctionDeclaration = {
            .id = NodeFunctionDeclaration,
            .trace = trace_start,
            .type = (void*) function_type,
            .identifier = info.identifier,
        }
    });
    declaration->body = info.generics_collection.generic_declarations_scope ? : new_scope(NULL);
    declaration->body->declaration = (void*) declaration;

    function_type->declaration = declaration;
    function_type->declaration->identifier.parent_declaration = (void*) declaration;

    assign_generics_to_declaration((void*) declaration, info.generics_collection);
    push(&parser->stack, declaration->body);

    if(info.generics_collection.generic_declarations_scope) {
        RecycleAccumulator accumulator = {
            parser->tokenizer->messages, info.generics_collection.generic_declarations_scope
        };
        traverse_type(return_type, NULL, &recycle_missing_generics, &accumulator,
                      TraverseGenerics | TraverseIntermediate, 0);
        traverse_action((Action) { ActionApplyCollection, .collection = info.declaration_actions },
                        &recycle_missing_generics, &accumulator, TraverseGenerics | TraverseIntermediate, 0);
    }

    Declaration* scoped_declaration = info.declaration_actions
                                          ? (void*) new_node((Node) {
                                              .Declaration.DeclarationLink = {
                                                  .id = NodeDeclarationLink,
                                                  .link = (void*) declaration,
                                                  .actions = info.declaration_actions,
                                              },
                                          })
                                          : (void*) declaration;

    put(&info.declaration_scope->variables, info.identifier.base, scoped_declaration);
    if(info.trait_scope) {
        put(&info.trait_scope->variables, info.identifier.base, scoped_declaration);
    }

    parse_function_arguments(function_type, declaration, parser, false);

    bool parse_body = false;

    if(no_body) {
        if(try(parser->tokenizer, '{', NULL)) parse_body = true;
    } else if(!declaration->identifier.is_external) {
        expect(parser->tokenizer, '{');
        parse_body = true;
    }

    if(info.declaration_scope->declaration->id == NodeStructDeclaration
       && info.declaration_scope->declaration->type->StructType.is_trait && parse_body) {
        push(&info.declaration_scope->declaration->StructDeclaration.trait_declarations, (void*) declaration);
    }

    if(info.trait_scope) {
        apply_action((Action) { ActionApplyCollection, .collection = info.declaration_actions }, 0, 2);
        // clash_types((void*) function_type, info.value->Variable.declaration->type, function_type->trace,
        //             parser->tokenizer->messages, 0);
        remove_action((Action) { ActionApplyCollection, .collection = info.declaration_actions }, 0, 2);
    }

    if(parse_body) {
        declaration->body->children = collect_until(parser, &statement, 0, '}');
    } else {
        expect(parser->tokenizer, ';');
    }

    close_generics_declaration((void*) declaration);
    pop(&parser->stack);

    return (void*) variable_of((void*) declaration, declaration->trace, fIgnoreStatement | fStatementTerminated);
}

Node* parse_function_lambda(Type* return_type, Parser* parser) {
    next(parser->tokenizer);
    static unsigned lambda_id = 0;

    FunctionType* function_type = (void*) new_type((Type) {
        .FunctionType = {
            .id = NodeFunctionType,
            .trace = return_type->trace,
        }
    });
    push(&function_type->signature, return_type);

    FunctionDeclaration* declaration = (void*) new_node((Node) {
        .FunctionDeclaration = {
            .id = NodeFunctionDeclaration,
            .trace = return_type->trace,
            .type = (void*) function_type,
            .identifier = {
                .base = strf(0, "__qlambda%u", lambda_id++),
                .parent_scope = (void*) last(parser->stack),
            }
        }
    });
    declaration->identifier.parent_declaration = (void*) declaration;
    declaration->body = new_scope((void*) declaration);
    function_type->declaration = declaration;

    push(&parser->stack, declaration->body);
    parse_function_arguments(function_type, declaration, parser, true);

    if(declaration->arguments && !last(declaration->arguments).identifier.len) {
        pop(&parser->stack);
        return (void*) function_type;
    }

    Token operator;
    switch((operator = next(parser->tokenizer)).type) {
        case TokenDoubleRightArrow: {
            Node* return_statement = new_node((Node) {
                .ReturnStatement = {
                    .id = NodeReturnStatement,
                    .value = expression(parser),
                },
            });

            push(&declaration->body->children, return_statement);
            break;
        }

        case '{': {
            declaration->body->children = collect_until(parser, &statement, 0, '}');
            break;
        }

        default:
            if(!declaration->arguments) {
                pop(&parser->stack);
                return (void*) function_type;
            }

            push(parser->tokenizer->messages, MERROR(operator.trace,
                     iftty(str("expected either "HERR"=>"H" or "HERR"{"H), str("expected either => or {"))));
    }

    pop(&parser->stack);
    return (void*) variable_of((void*) declaration, declaration->trace, 0);
}
