#ifndef EXTERNAL_H
#define EXTERNAL_H

#include <vector-string.h>

#include "../nodes.h"

typedef struct External {
    NODE_FIELDS;
    String data;
} External;

#endif