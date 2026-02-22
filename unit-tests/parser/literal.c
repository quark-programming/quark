#include <parser/parser.h>
#include <parser/lefthand/lefthand.h>
#include <parser/statement/statement.h>
#include "parser/statement/scope.h"
#include "../unit-tests.h"

int test_literal() {
    Vec(Message) messages = { 0 };

    test("numeric literals") {
        Tokenizer tokenizer = new_tokenizer("TEST LITERAL", "45 56", &messages);
        Parser parser = { &tokenizer };

        NumericLiteral* node = (void*) lefthand_expression(&parser);
        assert_eq(node->id, NodeNumericLiteral);
        assert_eq(node->value, 45);
        unbox((void*) node);

        node = (void*) lefthand_expression(&parser);
        assert_eq(node->id, NodeNumericLiteral);
        assert_eq(node->value, 56);
        unbox((void*) node);

        assert_eq(tokenizer.current.type, 0);
        assert_eq(len(messages), 0);
    }

    test("auto type literals") {
        Tokenizer tokenizer = new_tokenizer("TEST LITERAL", "auto int", &messages);
        Parser parser = { &tokenizer };

        Wrapper* node = (void*) lefthand_expression(&parser);
        assert_eq(node->id, WrapperAuto);
        assert_eq(node->Auto.ref, NULL);
        assert_eq((bool) (node->flags & fNumeric), false);
        unbox((void*) node);

        node = (void*) lefthand_expression(&parser);
        assert_eq(node->id, WrapperAuto);
        assert_eq(node->Auto.ref, NULL);
        assert_eq((bool) (node->flags & fNumeric), true);
        unbox((void*) node);

        assert_eq(tokenizer.current.type, 0);
        assert_eq(len(messages), 0);
    }

    test("external literal") {
        Tokenizer tokenizer = new_tokenizer("TEST LITERAL", "extern x extern int x extern<auto> x", &messages);
        Parser parser = { &tokenizer };

        External* node = (void*) lefthand_expression(&parser);
        assert_eq(node->id, NodeExternal);
        assert_eq(node->flags & fType && !(node->flags & fNumeric), true);
        assert_eq(streq(node->data, str("x")), true);
        unbox((void*) node);

        node = (void*) lefthand_expression(&parser);
        assert_eq(node->id, NodeExternal);
        assert_eq(node->flags & fType && node->flags & fNumeric, true);
        assert_eq(streq(node->data, str("x")), true);
        unbox((void*) node);

        node = (void*) lefthand_expression(&parser);
        assert_eq(node->id, NodeExternal);
        assert_eq((bool) (node->flags & fType), false);
        assert_eq(streq(node->data, str("x")), true);
        unbox((void*) node);

        assert_eq(tokenizer.current.type, 0);
        assert_eq(len(messages), 0);
    }

    test("structure literal") {
        Tokenizer tokenizer = new_tokenizer("TEST LITERAL", "struct S {} S {}", &messages);
        Parser parser = { &tokenizer };
        push(&parser.stack, new_scope(NULL));

        Node* empty = statement(&parser);
        assert_eq(empty->id, NodeNone);
        unbox((void*) empty);

        Node* node = (void*) lefthand_expression(&parser);
        assert_eq(node->id, NodeStructLiteral);
        assert_eq(streq(node->type->Wrapper.Variable.declaration->identifier.base, str("S")), true);
        unbox((void*) node);

        assert_eq(tokenizer.current.type, 0);
        assert_eq(len(messages), 0);
    }

    test("array literal") {
        Tokenizer tokenizer = new_tokenizer("TEST LITERAL", "struct Slice<T> {} [1, 2, 3]", &messages);
        Parser parser = { &tokenizer };
        push(&parser.stack, new_scope(NULL));

        unbox(statement(&parser));

        StructLiteral* node = (void*) lefthand_expression(&parser);
        assert_eq(node->id, NodeStructLiteral);
        assert_eq(len(node->field_values), 2);
        assert_eq(node->field_values[0]->id, NodeStructLiteral);
        assert_eq(len(node->field_values[0]->StructLiteral.field_values), 3);
        assert_eq(node->type->id, WrapperVariable);
        assert_eq((bool) (node->type->Wrapper.action.generics[0]->flags & fNumeric), true);
    }

    test("typeof()") {
        Tokenizer tokenizer = new_tokenizer("TEST LITERAL", "typeof(5)", &messages);
        Parser parser = { &tokenizer };

        Wrapper* type = (void*) lefthand_expression(&parser);
        assert_eq(type->id, WrapperAuto);
        assert_eq((bool) (type->flags & fNumeric), true);
    }

    return print_result();
}
