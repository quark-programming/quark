#include "generics.h"
#include "clash_types.h"
#include "types.h"

#include "../righthand/righthand.h"
#include "../righthand/declaration/declarations.h"
#include "../statement/statement.h"
#include "../statement/scope.h"

Type* wrap_applied_generics(Type* type, const TypeVector generics, Declaration* declaration) {
    return new_type((Type) {
        .Wrapper = {
            .id = NodeWrapper,
            .trace = type->trace,
            .flags = type->flags,
            .action = { ActionApplyCollection, generics, (void*) declaration },
            .Auto = { type },
        }
    });
}

void apply_type_arguments(Wrapper* variable, Parser* parser) {
    while(variable->id != WrapperVariable) variable = (void*) variable->child;
    Declaration* const declaration = variable->Variable.declaration;
    if(!declaration->generics.base_type_arguments.size) return;

    const TypeVector base_generics = declaration->generics.base_type_arguments;
    TypeVector input_generics = { 0 };

    for(size_t i = 0; i < base_generics.size; i++) {
        Type* const type_argument = new_type((Type) {
            .Wrapper = {
                .id = NodeWrapper,
                .trace = variable->trace,
                .flags = base_generics.data[i]->flags,
                .Auto = { .parent_base_generic = base_generics.data[i] },
            }
        });
        push(&input_generics, type_argument);
    }

    if(try(parser->tokenizer, '<', 0)) {
        global_righthand_collecting_type_arguments = true;
        const NodeVector type_arguments = collect_until(parser, &expression, ',', '>');
        global_righthand_collecting_type_arguments = false;
        for(size_t i = 0; i < type_arguments.size; i++) {
            if(!(type_arguments.data[i]->flags & fType)) {
                push(parser->tokenizer->messages, REPORT_ERR(type_arguments.data[i]->trace,
                         String("expected a type in type arguments")));
            }

            if(i >= base_generics.size) {
                push(parser->tokenizer->messages, REPORT_ERR(stretch(type_arguments.data[i]->trace,
                         last(type_arguments)->trace), String("too many type arguments")));
                push(parser->tokenizer->messages, see_declaration(declaration, type_arguments.data[i]->trace));
                break;
            }

            clash_types(input_generics.data[i], (void*) type_arguments.data[i], type_arguments.data[i]->trace,
                        parser->tokenizer->messages, 0);
            input_generics.data[i]->trace = type_arguments.data[i]->trace;
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

        GenericReference* generic_type = (void*) new_type((Type) {
            .GenericReference = {
                .id = NodeGenericReference,
                .flags = fConstExpr,
                .trace = identifier.trace,
                .index = collection.base_type_arguments.size,
            }
        });
        push(&collection.declaration_setters, &generic_type->generics_declaration);

        Wrapper* base_generic = (void*) new_type((Type) {
            .Wrapper = {
                .id = NodeWrapper,
                .flags = fConstExpr,
                .trace = identifier.trace,
                .Auto.replacement_generic = (void*) new_type((Type) {
                    .GenericReference = {
                        .id = NodeGenericReference,
                        .flags = fConstExpr,
                        .trace = identifier.trace,
                        .index = collection.base_type_arguments.size,
                    }
                }),
            }
        });
        push(&collection.base_type_arguments, (void*)base_generic);
        push(&collection.declaration_setters, &base_generic->Auto.replacement_generic->generics_declaration);

        Declaration* const generic_variable_declaration = (void*) new_node((Node) {
            .VariableDeclaration = {
                .id = NodeVariableDeclaration,
                .trace = identifier.trace,
                .flags = fConst | fType,
                .type = (void*) generic_type,
                .const_value = (void*) generic_type,
            }
        });
        put(&collection.generic_declarations_scope->variables, identifier.trace.source, generic_variable_declaration);

        if(!try(parser->tokenizer, ',', 0)) break;
    }
    expect(parser->tokenizer, '>');

    return collection;
}

void assign_generics_to_declaration(Declaration* declaration, GenericsCollection collection) {
    if(!collection.base_type_arguments.size) return;
    declaration->generics.base_type_arguments = collection.base_type_arguments;
    push(&declaration->generics.type_arguments_stack, collection.base_type_arguments);

    for(size_t i = 0; i < collection.declaration_setters.size; i++) {
        *collection.declaration_setters.data[i] = declaration;
    }
}

void close_generics_declaration(Declaration* declaration) {
    declaration->generics.base_type_arguments.size = 0;
    // TODO: possible fix? remove anchors
}
