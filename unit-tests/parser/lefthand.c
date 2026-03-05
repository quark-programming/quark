#include <parser/lefthand/lefthand.h>
#include <parser/statement/statement.h>
#include "../unit-tests.h"
#include "parser/righthand/righthand.h"
#include "parser/statement/scope.h"

int test_lefthand() {
    Vec(Message) messages = { 0 };
    Parser parser = { 0 };
    push(&parser.stack, new_scope(NULL));

    test("reference and dereference") {
        Tokenizer tokenizer = new_tokenizer("TEST LEFTHAND", "int x; &x int* y; *y *x &0", &messages);
        parser.tokenizer = &tokenizer;

        unbox(statement(&parser));

        Wrapper* reference = (void*) lefthand_expression(&parser);
        (void) reference;
        // assert_eq(reference->id, WrapperSurround);
        // assert_eq(streq(reference->Surround.prefix, str("(&")), true);
        // assert_eq(reference->type->id, NodePointerType);

        unbox(statement(&parser));

        Wrapper* dereference = (void*) lefthand_expression(&parser);
        assert_eq(dereference->id, WrapperSurround);
        assert_eq(streq(dereference->Surround.prefix, str("(*")), true);
        assert_eq(dereference->type->id != NodePointerType, true);

        unbox(lefthand_expression(&parser));
        assert_eq(len(messages), 1);
        pop(&messages);
        unbox(lefthand_expression(&parser));
        // TODO: references should only work with `fMutable` or some other flag
        // assert_eq(messages.size, 1);
        // pop(&messages);

        assert_eq(len(messages), 0);
        assert_eq(tokenizer.current.type, 0);
    }

    test("type modifiers") {
        Tokenizer tokenizer = new_tokenizer("TEST LEFTHAND", "auto const auto", &messages);
        parser.tokenizer = &tokenizer;

        Wrapper* non_const = (void*) lefthand_expression(&parser);
        assert_eq(non_const->type->id, WrapperAuto);
        assert_eq((bool) (non_const->flags & fConst), false);
        unbox((void*) non_const);

        Wrapper* const_node = (void*) lefthand_expression(&parser);
        assert_eq(const_node->type->id, WrapperAuto);
        assert_eq((bool) (const_node->flags & fConst), true);
        unbox((void*) const_node);

        assert_eq(len(messages), 0);
        assert_eq(tokenizer.current.type, 0);
    }

    test("parentheses") {
        Tokenizer tokenizer = new_tokenizer("TEST LEFTHAND", "(1 + 2) * 3", &messages);
        parser.tokenizer = &tokenizer;

        BinaryOperation* operation = (void*) righthand_expression(lefthand_expression(&parser), &parser, 15);
        assert_eq(operation->id, NodeBinaryOperation);
        assert_eq(streq(operation->operator, str("*")), true);

        assert_eq(operation->left->id, WrapperSurround);
        assert_eq(operation->left->Wrapper.Surround.child->id, NodeBinaryOperation);

        assert_eq(len(messages), 0);
        assert_eq(tokenizer.current.type, 0);
    }

    return print_result();
}
