#include "generics.h"
#include "clash_types.h"
#include "types.h"

#include "../righthand/righthand.h"
#include "../righthand/declaration/declaration.h"
#include "../statement/statement.h"
#include "../statement/scope.h"
#include "parser/lefthand/lefthand.h"

Node* assign_action(Node* node, Action action, const bool important, const bool owned_variable) {
    if(!owned_variable) {
        return (void*) new_type((Type) {
            .Wrapper = {
                .id = WrapperAuto,
                .trace = node->trace,
                .flags = node->flags,
                .action = action,
                .Auto.ref = (void*) node,
            },
        });
    }

    if(node->Wrapper.action.type) {
        node->Wrapper.action = (Action) {
            .type = ActionApplyCollection,
            .collection = important ? vec(action, node->Wrapper.action) : vec(node->Wrapper.action, action),
        };
    } else {
        node->Wrapper.action = action;
    }

    node->Wrapper.type = (void*) assign_action((void*) node->Wrapper.type, action, important, false);
    return node;
}

void apply_type_arguments(Wrapper* variable, Parser* parser) {
    while(variable->id != WrapperVariable) variable = (void*) variable->child;
    Declaration* const declaration = variable->Variable.declaration;
    if(!declaration->generics.base_type_arguments) return;

    Vec(Type*) const base_generics = declaration->generics.base_type_arguments;
    Vec(Type*) input_generics = NULL;

    for(size_t i = 0; i < len(base_generics); i++) {
        Type* const type_argument = new_type((Type) {
            .Wrapper = {
                .id = WrapperAuto,
                .trace = variable->trace,
                .flags = base_generics[i]->flags,
                .Auto = {
                    .test_against = base_generics[i]
                },
            }
        });
        push(&input_generics, type_argument);
    }

    if(try(parser->tokenizer, '<', 0)) {
        const bool save = global_righthand_collecting_type_arguments;
        global_righthand_collecting_type_arguments = true;

        Vec(Type*) type_arguments = NULL;

        while(parser->tokenizer->current.type && parser->tokenizer->current.type != '>'
              && parser->tokenizer->current.type != TokenDoubleGreater) {
            push(&type_arguments, get_type(parser));
            if(!try(parser->tokenizer, ',', NULL)) break;
        }

        Token double_greater;
        if(try(parser->tokenizer, TokenDoubleGreater, &double_greater)) {
            parser->tokenizer->current.trace.source = (str) {
                .len = 1,
                .data = double_greater.trace.source.data + 1,
            };
            parser->tokenizer->current.type = '>';
        } else expect(parser->tokenizer, '>');

        global_righthand_collecting_type_arguments = save;

        for(u32 i = 0; i < len(type_arguments); i++) {
            if(i >= len(base_generics)) {
                push(parser->tokenizer->messages, MERROR(stretch(type_arguments[i]->trace,
                         last(type_arguments)->trace), str("excess type arguments")));
                push(parser->tokenizer->messages, see_declaration(declaration, type_arguments[i]->trace));
                break;
            }

            clash_types(input_generics[i], (void*) type_arguments[i], type_arguments[i]->trace,
                        parser->tokenizer->messages, 0);
            input_generics[i]->trace = type_arguments[i]->trace;
        }
    }

    assign_action((void*) variable, (Action) { ActionApplyGenerics, input_generics, declaration }, false, true);
}

GenericsCollection collect_generics(Parser* const parser) {
    GenericsCollection collection = { 0 };

    if(!try(parser->tokenizer, '<', 0)) {
        return collection;
    }

    collection.generic_declarations_scope = new_scope(NULL);
    push(&parser->stack, collection.generic_declarations_scope);

    while(parser->tokenizer->current.type && parser->tokenizer->current.type != '>') {
        const Token identifier = expect(parser->tokenizer, TokenIdentifier);

        Type* const base_type = new_type((Type) {
            .Wrapper = {
                .id = WrapperAuto,
                .flags = fConstExpr,
                .trace = identifier.trace,
                .Auto.priority = -1,
            },
        });

#define exp_trait_error() \
    push(parser->tokenizer->messages, MERROR(extension->trace, str("expected a trait or structure here")))

        if(try(parser->tokenizer, ':', NULL)) {
            do {
                Wrapper* const extension = (void*) lefthand_expression(parser);
                if(extension->id != WrapperVariable) {
                    exp_trait_error();
                    continue;
                }

                Declaration* const declaration = extension->Variable.declaration;
                switch(declaration->id) {
                    case NodeFunctionDeclaration: {
                        StructDeclaration* const parent_trait =
                                (void*) declaration->identifier.parent_scope->declaration;
                        if(parent_trait->id != NodeStructDeclaration || !parent_trait->type->StructType.is_trait) {
                            exp_trait_error();
                            break;
                        }

                        push(&base_type->Wrapper.Auto.required_traits, declaration);
                        break;
                    }

                    case NodeStructDeclaration:
                        if(!declaration->type->StructType.is_trait) {
                            exp_trait_error();
                            break;
                        }

                        push(&base_type->Wrapper.Auto.required_traits, declaration);
                        break;

                    default: exp_trait_error();
                }
            } while(try(parser->tokenizer, '+', NULL));
        }

#undef exp_trait_error

        Declaration* const type_declaration = (void*) new_node((Node) {
            .VariableDeclaration = {
                .id = NodeVariableDeclaration,
                .flags = fType | fConst,
                .type = base_type,
                .const_value = (void*) base_type,
            },
        });

        put(&collection.generic_declarations_scope->variables, identifier.trace.source, type_declaration);
        push(&collection.base_type_arguments, base_type);

        if(!try(parser->tokenizer, ',', 0)) break;
    }
    expect(parser->tokenizer, '>');
    pop(&parser->stack);

    return collection;
}

void assign_generics_to_declaration(Declaration* declaration, const GenericsCollection collection) {
    if(!len(collection.base_type_arguments)) return;
    declaration->generics.base_type_arguments = collection.base_type_arguments;
    push(&declaration->generics.type_arguments_stacks[0], collection.base_type_arguments);
    push(&declaration->generics.type_arguments_stacks[1], collection.base_type_arguments);
}

void close_generics_declaration(Declaration* declaration) {
    len(declaration->generics.type_arguments_stacks[0]) = 0;
    len(declaration->generics.type_arguments_stacks[1]) = 0;

    for(size_t i = 0; i < len(declaration->generics.base_type_arguments); i++) {
        Type* base_type = declaration->generics.base_type_arguments[i];
        declaration->generics.base_type_arguments[i] = new_type(*base_type);

        *base_type = (Type) {
            .GenericReference = {
                .id = NodeGenericReference,
                .flags = base_type->flags,
                .trace = base_type->trace,
                .generics_declaration = declaration,
                .index = i,
            },
        };
        base_type->type = base_type;
    }
}
