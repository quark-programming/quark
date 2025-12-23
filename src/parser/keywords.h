#ifndef KEYWORDS_H
#define KEYWORDS_H

#include <hashmap.h>

#include "nodes/nodes.h"
#include "parser.h"

typedef HashMap(Keyword) KeywordTable;

extern KeywordTable global_keyword_table;

#endif