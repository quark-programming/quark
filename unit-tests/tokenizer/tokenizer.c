#include <tokenizer/tokenizer.h>
#include "../unit-tests.h"

int test_tokenizer() {
    Vec(Message) messages = { 0 };

    test("basic tokens") {
        Tokenizer tokenizer = new_tokenizer("TEST TOKENIZER", "a b c 1 2 3 def 456", &messages);

        static char types[] = {
            TokenIdentifier, TokenIdentifier, TokenIdentifier,
            TokenNumber, TokenNumber, TokenNumber,
            TokenIdentifier, TokenNumber,
        };
        static char* strings[] = { "a", "b", "c", "1", "2", "3", "def", "456" };

        for(int i = 0; i < sizeof(types) / sizeof(*types); i++) {
            const Token token = next(&tokenizer);
            assert_eq(token.type, types[i]);
            assert_eq(streq(token.trace.source, ((str) { strlen(strings[i]), strings[i] })), true);
        }

        assert_eq(tokenizer.current.type, 0);
        assert_eq(len(messages), 0);
    }

    test("skips comments") {
        Tokenizer tokenizer = new_tokenizer("TEST TOKENIZER",
                                            (char[]) { "// Hello World\n 67 /* <- funny number */ 87" }, &messages);

        const Token first = next(&tokenizer);
        assert_eq(first.type, TokenNumber);
        assert_eq(streq(first.trace.source, str("67")), true);

        const Token second = next(&tokenizer);
        assert_eq(second.type, TokenNumber);
        assert_eq(streq(second.trace.source, str("87")), true);

        assert_eq(tokenizer.current.type, 0);
        assert_eq(len(messages), 0);
    }

    test("complex tokens") {
        Tokenizer tokenizer = new_tokenizer("TEST TOKENIZER", "\"string\" :: +=", &messages);

        static char types[] = { TokenString, TokenDoubleColon, TokenPlusEqual };
        static char* strings[] = { "\"string\"", "::", "+=" };

        for(int i = 0; i < sizeof(types) / sizeof(*types); i++) {
            const Token token = next(&tokenizer);
            assert_eq(token.type, types[i]);
            assert_eq(streq(token.trace.source, ((str) { strlen(strings[i]), strings[i] })), true);
        }

        assert_eq(tokenizer.current.type, 0);
        assert_eq(len(messages), 0);
    }

    return print_result();
}
