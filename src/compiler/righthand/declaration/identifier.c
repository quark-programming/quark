#include "identifier.h"

#include "../../../parser/type/stringify_type.h"

Map(Declaration*) global_declaration_space = NULL;

void populate_global_c_keywords() {
    // https://en.cppreference.com/w/c/keyword.html
    char* keywords[] = {
        "alignas", "alignas", "alignof", "auto", "bool", "break", "case", "char", "const", "constexpr", "continue",
        "default", "do", "double", "else", "enum", "extern", "false", "float", "for", "goto", "if", "inline", "int",
        "long", "nullptr", "register", "restrict", "return", "short", "signed", "sizeof", "static", "static_assert",
        "struct", "switch", "thread_local", "true", "typedef", "typeof", "typeof_unequal", "union", "unsigned", "void",
        "volatile", "while", "_Alignas", "_Alignof", "_Atomic", "_BitInt", "_Bool", "_Complex", "_Decimal128",
        "_Decimal32", "_Decimal64", "_Generic", "_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local",
    };

    for(int i = 0; i < sizeof(keywords) / sizeof(char*); i++) {
        put(&global_declaration_space, ((str) { strlen(keywords[i]), keywords[i] }), NULL);
    }
}

Map(Declaration*) global_function_identifiers = 0;

static void prevent_keyword(String* const identifier_builder) {
    Declaration** defined_declaration = get(global_declaration_space, as_str(*identifier_builder));
    if(defined_declaration && !*defined_declaration) {
        strf(identifier_builder, "_");
    }
}

void build_simple_identifier(const str identifier, String* const application) {
    String identifier_builder = { 0 };

    strf(&identifier_builder, "%.*s", fmtof(identifier));
    prevent_keyword(&identifier_builder);

    strf(application, "%.*s", fmtof(identifier_builder));
    free(vbase(identifier_builder));
}

static void build_identifier_base(const Identifier identifier, String* const identifier_builder) {
    if(!identifier.is_external && !(identifier.parent_declaration->id == NodeVariableDeclaration
                                    && identifier.parent_declaration->VariableDeclaration.compilation_state ==
                                    CompilationHoisted)) {
        if(identifier.parent_scope && identifier.parent_scope->id == NodeFunctionDeclaration) {
            const Identifier parent_ident = identifier.parent_scope->FunctionDeclaration.identifier;
            build_identifier_base(parent_ident, identifier_builder);
            strf(identifier_builder, "__");
        }

        if(identifier.parent_scope && identifier.parent_scope->id == NodeStructType
           && !(identifier.parent_declaration->id == NodeVariableDeclaration
                && !(identifier.parent_declaration->type->flags & fType))) {
            const Identifier parent_ident = ((StructType*) (void*) identifier.parent_scope)->parent->identifier;
            build_identifier_base(parent_ident, identifier_builder);
            strf(identifier_builder, "__");
        }

        if(identifier.reference_structure) {
            const Identifier parent_ident = identifier.reference_structure->parent->identifier;
            build_identifier_base(parent_ident, identifier_builder);
            strf(identifier_builder, "__");
        }
    }

    strf(identifier_builder, "%.*s", fmtof(identifier.base));

    if(len(identifier.parent_declaration->generics.type_arguments_stack) && !identifier.is_external) {
        stringify_generics(identifier_builder, last(identifier.parent_declaration->generics.type_arguments_stack),
                           StringifyAlphaNumeric);
    }
}

void build_full_identifier(const Identifier identifier, String* const application) {
    String identifier_builder = { 0 };

    build_identifier_base(identifier, &identifier_builder);
    prevent_keyword(&identifier_builder);

    strf(application, "%.*s", fmtof(identifier_builder));
    free(vbase(identifier_builder));
}

bool resolve_identifier(const Identifier identifier, String* const identifier_builder) {
    build_identifier_base(identifier, identifier_builder);

    Declaration** defined_declaration;
    while(((defined_declaration = get(global_declaration_space, as_str(*identifier_builder))))
          && *defined_declaration != identifier.parent_declaration) {
        strf(identifier_builder, "_");
    }

    if(!defined_declaration) {
        put(&global_declaration_space, as_str(*identifier_builder), identifier.parent_declaration);
    }

    return defined_declaration;
}
