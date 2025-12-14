#ifndef NODE_EXTERNAL_H
#define NODE_EXTERNAL_H

#include <vector-string.h>

#include "../fields.h"

typedef struct External {
    NODE_FIELDS;
    String data;
} External;

#endif