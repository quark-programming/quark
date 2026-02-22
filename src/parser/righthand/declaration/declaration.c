#include "declaration.h"

#include "function.h"
#include "identifier.h"
#include "variable.h"

Message see_declaration(Declaration* declaration, Trace trace) {
    if(!declaration) {
        return MINFO({ 0 }, str("declaration not found"));
    }
    return MINFO(declaration->trace, strf(0, "declaration of '\33[35m%.*s\33[0m'", fmtof(trace.source)));
}

Node* parse_declaration(Node* type, Token identifier, Parser* parser) {
    if(!(type->flags & fType)) {
        push(parser->tokenizer->messages,
             MERROR(type->trace, str("expected a type before declaration identifier")));
        push(parser->tokenizer->messages, MHINT(str("also try '\33[35mtypeof(expr)\33[0m'")));
        type = (void*) type->type;
    }

    const IdentifierInfo info = new_identifier(identifier, parser, IdentifierDeclaration);

    if(try(parser->tokenizer, '(', 0)) {
        return parse_function_declaration((void*) type, info, parser);
    }

    return parse_variable_declaration((void*) type, info, parser);
}
