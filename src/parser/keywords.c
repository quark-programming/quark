#include "keywords.h"
#include "lefthand/keywords.h"

KeywordTable global_keyword_table = 0;

// TODO: call in main(), assign token.identifier in [./block.c]
void populate_keyword_table() {
    put(&global_keyword_table, String("auto"), (Keyword) { KeywordActionNone, &keyword_auto });
    put(&global_keyword_table, String("int"), (Keyword) { KeywordActionNone, &keyword_int });
    put(&global_keyword_table, String("typeof"), (Keyword) { KeywordActionNone, &keyword_typeof });
    put(&global_keyword_table, String("sizeof"), (Keyword) { KeywordActionNone, &keyword_sizeof });
    put(&global_keyword_table, String("const"), (Keyword) { KeywordActionNone, &keyword_const });
    put(&global_keyword_table, String("extern"), (Keyword) { KeywordActionNone, &keyword_extern });
    put(&global_keyword_table, String("self"), (Keyword) { KeywordActionSelf, &keyword_self });
}