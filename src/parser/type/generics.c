#include "generics.h"
#include "clash_types.h"
#include "types.h"

#include "../righthand/righthand.h"
#include "../righthand/declaration/declaration.h"
#include "../statement/statement.h"
#include "../statement/scope.h"

Type* wrap_applied_generics(Type* type, Vec(Type*) const generics, Declaration* declaration) {
    return new_type((Type) {
        .Wrapper = {
            .id = WrapperAuto,
            .trace = type->trace,
            .flags = type->flags,
            .action = { ActionApplyGenerics, generics, (void*) declaration },
            .Auto = { type },
        }
    });
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
        Vec(Node*) const type_arguments = collect_until(parser, &expression, ',', '>');
        global_righthand_collecting_type_arguments = save;

        for(size_t i = 0; i < len(type_arguments); i++) {
            if(!(type_arguments[i]->flags & fType)) {
                push(parser->tokenizer->messages, MERROR(type_arguments[i]->trace,
                         str("expected a type in type arguments")));
            }

            if(i >= len(base_generics)) {
                push(parser->tokenizer->messages, MERROR(stretch(type_arguments[i]->trace,
                         last(type_arguments)->trace), str("too many type arguments")));
                push(parser->tokenizer->messages, see_declaration(declaration, type_arguments[i]->trace));
                break;
            }

            clash_types(input_generics[i], (void*) type_arguments[i], type_arguments[i]->trace,
                        parser->tokenizer->messages, 0);
            input_generics[i]->trace = type_arguments[i]->trace;
        }
    }

    variable->action = (Action) { ActionApplyGenerics, input_generics, (void*) declaration };
    variable->type = wrap_applied_generics(variable->type, input_generics, declaration);
}

GenericsCollection collect_generics(Parser* const parser) {
    GenericsCollection collection = { 0 };

    if(!try(parser->tokenizer, '<', 0)) {
        return collection;
    }

    collection.generic_declarations_scope = new_scope(NULL);

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
