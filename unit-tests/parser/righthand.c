#include <parser/righthand/righthand.h>
#include <parser/statement/scope.h>
#include <parser/statement/statement.h>
#include <parser/type/types.h>
#include "../unit-tests.h"

int test_righthand() {
    Vec(Message) messages = { 0 };
    Parser parser = { 0 };
    push(&parser.stack, new_scope(NULL));

    test("binary operation & order of operations") {
        Tokenizer tokenizer = new_tokenizer("TEST RIGHTHAND", "1 + 2 * 3", &messages);
        parser.tokenizer = &tokenizer;

        BinaryOperation* const expr = (void*) expression(&parser);
        assert_eq(expr->id, NodeBinaryOperation);
        assert_eq(streq(expr->operator, str("+")), true);

        assert_eq(expr->right->id, NodeBinaryOperation);
        assert_eq(streq(expr->right->BinaryOperation.operator, str("*")), true);

        unbox((void*) expr);

        assert_eq(tokenizer.current.type, 0);
        assert_eq(len(messages), 0);
    }

    test("binary '*' vs ternary '*'") {
        Tokenizer tokenizer = new_tokenizer("TEST RIGHTHAND", "int * x; 4 * 5", &messages);
        parser.tokenizer = &tokenizer;

        Node* ignored = (void*) statement(&parser);
        assert_eq(ignored->id, NodeNone);
        Wrapper* variable = (void*) ignored->type;
        assert_eq(variable->id, WrapperVariable);

        BinaryOperation* bin = (void*) expression(&parser);
        assert_eq(bin->id, NodeBinaryOperation);

        assert_eq(tokenizer.current.type, 0);
        assert_eq(len(messages), 0);
    }

    test("declarations, declarations as variables, and binary/externals type matching") {
        Tokenizer tokenizer = new_tokenizer("TEST RIGHTHAND", "extern a a; extern b b + a", &messages);
        parser.tokenizer = &tokenizer;

        unbox(statement(&parser));

        BinaryOperation* bin = (void*) expression(&parser);
        assert_eq(bin->id, NodeBinaryOperation);
        assert_eq(bin->left->id, WrapperVariable);
        assert_eq(bin->right->id, WrapperVariable);

        assert_eq(len(messages), 1);
        pop(&messages);

        assert_eq(tokenizer.current.type, 0);
    }

    test("assignment operator") {
        Tokenizer tokenizer = new_tokenizer("TEST RIGHTHAND", "int y = 5; y = 4; 4 = 4", &messages);
        parser.tokenizer = &tokenizer;

        StatementWrapper* decl_assignment = (void*) statement(&parser);
        assert_eq(decl_assignment->id, NodeStatementWrapper);
        assert_eq(decl_assignment->expression->id, NodeBinaryOperation);
        assert_eq(streq(decl_assignment->expression->BinaryOperation.operator, str("=")), true);

        StatementWrapper* assignment = (void*) statement(&parser);
        assert_eq(assignment->id, NodeStatementWrapper);
        assert_eq(assignment->expression->id, NodeBinaryOperation);
        assert_eq(streq(assignment->expression->BinaryOperation.operator, str("=")), true);
        assert_eq(len(messages), 0);

        BinaryOperation* failed_assignment = (void*) expression(&parser);
        assert_eq(failed_assignment->id, NodeBinaryOperation);
        assert_eq(streq(failed_assignment->operator, str("=")), true);
        assert_eq(len(messages), 1);
        pop(&messages);

        assert_eq(tokenizer.current.type, 0);
    }

    test("function calls & external functions") {
        Tokenizer tokenizer = new_tokenizer(
            "TEST RIGHTHAND", "extern b extern test(extern a a); auto z = test(5)", &messages);
        parser.tokenizer = &tokenizer;

        unbox(statement(&parser));

        BinaryOperation* bin = (void*) expression(&parser);
        assert_eq(bin->id, NodeBinaryOperation);

        assert_eq(bin->left->id, WrapperVariable);
        assert_eq(bin->left->type->id, WrapperAuto);
        assert_eq(bin->left->type->Wrapper.Auto.ref->id, NodeExternal);
        assert_eq(streq(bin->left->type->Wrapper.Auto.ref->External.data, str("b")), true);

        assert_eq(bin->right->id, NodeFunctionCall);
        assert_eq(len(messages), 1);
        pop(&messages);

        assert_eq(tokenizer.current.type, 0);
    }

    test("field access and pointer to field access") {
        Tokenizer tokenizer = new_tokenizer("TEST RIGHTHAND",
                                            "struct Fields { extern a a; }"
                                            "Fields fields;"
                                            "auto fields_a = fields.a;"
                                            "auto fields_ref_a = (&fields)->a;",
                                            &messages);
        parser.tokenizer = &tokenizer;
        collect_until(&parser, &statement, 0, 0);
        assert_eq(len(messages), 0);

        Wrapper* fields_a = find_in_scope(*parser.stack[0], (Trace) { str("fields_a") });
        assert_eq((bool) fields_a, true);
        External* fields_a_type = (void*) open_type(fields_a->type, 0).type;
        assert_eq(fields_a_type->id, NodeExternal);
        assert_eq(streq(fields_a_type->data, str("a")), true);

        Wrapper* fields_ref_a = find_in_scope(*parser.stack[0], (Trace) { str("fields_ref_a") });
        assert_eq((bool) fields_ref_a, true);
        External* fields_ref_a_type = (void*) open_type(fields_ref_a->type, 0).type;
        assert_eq(fields_ref_a_type->id, NodeExternal);
        assert_eq(streq(fields_ref_a_type->data, str("a")), true);
    }

    test("comparison operator precedence and resulting type") {
        Tokenizer tokenizer = new_tokenizer("TEST RIGHTHAND", "1 > 2 == 3", &messages);
        parser.tokenizer = &tokenizer;

        BinaryOperation* expr = (void*) expression(&parser);
        assert_eq(expr->id, NodeBinaryOperation);
        assert_eq(streq(expr->operator, str("==")), true);
        assert_eq(expr->type->id, NodeExternal);
        assert_eq(streq(expr->type->External.data, str("bool")), true);

        assert_eq(expr->left->id, NodeBinaryOperation);
        assert_eq(streq(expr->left->BinaryOperation.operator, str(">")), true);

        assert_eq(len(messages), 0);
        assert_eq(tokenizer.current.type, 0);
    }

    test("indexing pointers") {
        Tokenizer tokenizer = new_tokenizer("TEST RIGHTHAND", "int* pointer; pointer[42]", &messages);
        parser.tokenizer = &tokenizer;

        unbox(statement(&parser));

        Wrapper* deref = (void*) expression(&parser);
        assert_eq(deref->id, WrapperSurround);
        assert_eq(streq(deref->Surround.prefix, str("(*")), true);

        assert_eq(deref->Surround.child->id, WrapperSurround);
        assert_eq(deref->Surround.child->Wrapper.Surround.child->id, NodeBinaryOperation);
        assert_eq(deref->Surround.child->Wrapper.Surround.child->BinaryOperation.right->id, NodeNumericLiteral);
        assert_eq(deref->Surround.child->Wrapper.Surround.child->BinaryOperation.right->NumericLiteral.value, 42);

        assert_eq(len(messages), 0);
        assert_eq(tokenizer.current.type, 0);
    }

    test("optionals & optional coalescing") {
        Tokenizer tokenizer = new_tokenizer("TEST RIGHTHAND",
                                            "struct Option<T> { extern bool some; T value; }"
                                            "extern TEST? basic_option;"
                                            "Fields? object_option;"
                                            "auto coalesced_option = object_option?.a;",
                                            &messages);
        parser.tokenizer = &tokenizer;
        collect_until(&parser, &statement, 0, 0);
        assert_eq(len(messages), 0);

        Declaration* const Option = find_on_stack_unwrapped(parser.stack, str("Option"));
        assert_eq(Option != NULL, true);
        assert_eq(streq(Option->identifier.base, str("Option")), true);

        Declaration* const basic_option = find_on_stack_unwrapped(parser.stack, str("basic_option"));
        assert_eq(basic_option != NULL, true);
        assert_eq(basic_option->id, NodeVariableDeclaration);

        OpenedType open_basic_option = open_type(basic_option->type, 0);
        Type* basic_option_base = find_last_generic_action(open_basic_option.actions, Option)[0];
        close_type(open_basic_option.actions, 0);
        assert_eq(basic_option_base->id, NodeExternal);
        assert_eq(streq(basic_option_base->External.data, str("TEST")), true);

        Declaration* const coalesced_option = find_on_stack_unwrapped(parser.stack, str("coalesced_option"));
        assert_eq(coalesced_option != NULL, true);
        assert_eq(coalesced_option->id, NodeVariableDeclaration);

        OpenedType open_coalesced_option = open_type(coalesced_option->type, 0);
        Type* coalesced_option_base = find_last_generic_action(open_coalesced_option.actions, Option)[0];
        close_type(open_coalesced_option.actions, 0);
        assert_eq(coalesced_option_base->id, NodeExternal);
        assert_eq(streq(coalesced_option_base->External.data, str("a")), true);
    }

    return print_result();
}
