#include "declaration.h"

#include "function.h"
#include "identifier.h"
#include "tty.h"
#include "variable.h"

Message see_declaration(Declaration* declaration, Trace trace) {
    if(!declaration) {
        return MINFO({ 0 }, str("declaration not found"));
    }
    return MINFO(declaration->trace,
                 strf(0, iftty("declaration of "HINF"%.*s"H, "declaration of %.*s"), fmtof(trace.source)));
}

Node* parse_declaration(Node* type, Token identifier, Parser* parser) {
    if(!(type->flags & fType)) {
        push(parser->tokenizer->messages, MERROR(type->trace, str("expected a type before declaration identifier")));
        type = (void*) type->type;
    }

    const IdentifierInfo info = new_identifier(identifier, parser, IdentifierDeclaration);

    if(try(parser->tokenizer, '(', 0)) {
        return parse_function_declaration((void*) type, info, parser, false);
    }

    return parse_variable_declaration((void*) type, info, parser);
}

Declaration* create_declaration_link(Declaration* link, Scope* parent_scope, const u32 flags) {
    DeclarationLink* declaration = (void*) new_node(*(Node*)(void*) link);
    declaration->id = NodeDeclarationLink;
    declaration->link = link;
    declaration->identifier.parent_scope = parent_scope;
    declaration->flags |= flags;
    declaration->actions = NULL;
    return (void*) declaration;
}
