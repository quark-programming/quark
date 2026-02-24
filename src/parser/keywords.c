#include "keywords.h"

#include "lefthand/keywords.h"
#include "statement/keywords.h"

Map(Keyword) global_keyword_table = 0;

void populate_keyword_table() {
    put(&global_keyword_table, str("auto"), ((Keyword) { KeywordActionNone, &keyword_auto }));
    put(&global_keyword_table, str("int"), ((Keyword) { KeywordActionNone, &keyword_int }));
    put(&global_keyword_table, str("typeof"), ((Keyword) { KeywordActionNone, &keyword_typeof }));
    put(&global_keyword_table, str("sizeof"), ((Keyword) { KeywordActionNone, &keyword_sizeof }));
    put(&global_keyword_table, str("const"), ((Keyword) { KeywordActionNone, &keyword_const }));
    put(&global_keyword_table, str("extern"), ((Keyword) { KeywordActionNone, &keyword_extern }));
    put(&global_keyword_table, str("private"), ((Keyword) { KeywordActionNone, &keyword_private }));

    put(&global_keyword_table, str("import"), ((Keyword) { KeywordActionStatement, &keyword_import }));
    put(&global_keyword_table, str("return"), ((Keyword) { KeywordActionStatement, &keyword_return }));
    put(&global_keyword_table, str("struct"), ((Keyword) { KeywordActionStatement, &keyword_struct }));
    put(&global_keyword_table, str("trait"), ((Keyword) { KeywordActionStatement, &keyword_trait }));
    put(&global_keyword_table, str("if"), ((Keyword) { KeywordControlSingleCond, &keywords_control }));
    put(&global_keyword_table, str("while"), ((Keyword) { KeywordControlSingleCond, &keywords_control }));
    put(&global_keyword_table, str("for"), ((Keyword) { KeywordControlTripleCond, &keywords_control }));
    put(&global_keyword_table, str("type"), ((Keyword) { KeywordActionStatement, &keyword_type }));
}