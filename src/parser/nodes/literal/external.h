#ifndef NODE_EXTERNAL_H
#define NODE_EXTERNAL_H

#include "../fields.h"

typedef struct External {
    NODE_FIELDS;
    str data;
} External;

#endif