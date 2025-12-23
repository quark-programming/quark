#ifndef RIGHTHAND_H
#define RIGHTHAND_H

#include "../nodes/nodes.h"
#include "../parser.h"

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



extern bool global_righthand_collecting_type_arguments;

Node* expression(Parser* parser);

Node* righthand_expression(Node* lefthand, Parser* parser, unsigned char precedence);

#endif
