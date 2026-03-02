#ifndef NODE_DECLARATION_LINK_H
#define NODE_DECLARATION_LINK_H

#include "../../fields.h"

typedef struct DeclarationLink {
    DECLARATION_FIELDS;
    Declaration* link;
    Vec(struct Action) actions;
} DeclarationLink;

#endif