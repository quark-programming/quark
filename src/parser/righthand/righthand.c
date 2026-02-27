#include "righthand.h"

#include "assignment.h"
#include "binary.h"
#include "field_access.h"
#include "function_call.h"
#include "optional_coalesce.h"
#include "../lefthand/lefthand.h"
#include "../lefthand/reference.h"
#include "declaration/declaration.h"
#include "declaration/function.h"

// https://en.cppreference.com/w/c/language/operator_precedence.html
RighthandOperator global_righthand_operator_table[128] = {
    [TokenDoublePlus] = { 1, RightPostfixAssignment }, [TokenDoubleMinus] = { 1, RightPostfixAssignment },
    ['['] = { 1, RightIndex }, ['('] = { 1, RightCall },
    ['.'] = { 1, RightFieldAccess }, [TokenRightArrow] = { 1, RightFieldAccess },
    ['?'] = { 1, RightOptional }, [TokenDoubleDot] = { 1, RightRange },

    ['*'] = { 3, RightPostfixOrBinary }, ['/'] = { 3 }, ['%'] = { 3 },

    ['+'] = { 4 }, ['-'] = { 4 },

    [TokenDoubleLess] = { 5 }, [TokenDoubleGreater] = { 5 },

    ['<'] = { 6, RightCompare }, ['>'] = { 6, RightCompare },
    [TokenLessEqual] = { 6, RightCompare }, [TokenGreaterEqual] = { 6, RightCompare },

    [TokenDoubleEqual] = { 7, RightCompare }, [TokenNotEqual] = { 7, RightCompare },

    ['&'] = { 8 }, ['^'] = { 9 }, ['|'] = { 10 },

    [TokenDoubleAnd] = { 11, RightCompare }, [TokenDoubleOr] = { 12, RightCompare },

    [TokenIdentifier] = { 13, RightDeclaration },

    ['='] = { 14, RightAssignment }, [TokenPlusEqual] = { 14, RightAssignment },
    [TokenMinusEqual] = { 14, RightAssignment }, [TokenTimesEqual] = { 14, RightAssignment },
    [TokenDivideEqual] = { 14, RightAssignment }, [TokenModEqual] = { 14, RightAssignment },
    [TokenAndEqual] = { 14, RightAssignment }, [TokenXorEqual] = { 14, RightAssignment },
    [TokenOrEqual] = { 14, RightAssignment },
};

char* global_righthand_override_table[128] = {
    ['+'] = "add", ['-'] = "subtract", ['*'] = "multiply", ['/'] = "divide", ['%'] = "modulo",
    [TokenDoubleEqual] = "equals",
};

bool global_righthand_collecting_type_arguments = false;

Node* expression(Parser* parser) {
    return righthand_expression(lefthand_expression(parser), parser, 15);
}

Node* righthand_expression(Node* lefthand, Parser* parser, const unsigned char precedence) {
    RighthandOperator operator;
    while((operator = global_righthand_operator_table[parser->tokenizer->current.type]).precedence) {
        if(lefthand->flags & fStatementTerminated) break;
        if(operator.precedence >= precedence + (operator.type == RightAssignment)) break;
        if((operator.precedence == 6 || operator.precedence == 5) && global_righthand_collecting_type_arguments) break;

        switch(operator.type) {
            case RightOptional: {
                Node* const result = parse_optional_coalescing(lefthand, parser);
                if(result) lefthand = result;
                break;
            }

            case RightPostfixAssignment:
                lefthand = parse_postfix_assignment(lefthand, parser);
                break;

            case RightPostfixOrBinary: {
                Node* override = try_binary_postfix(lefthand, parser);
                lefthand = override ? override : parse_binary_operation(lefthand, operator, parser);
                break;
            }

            case RightAssignment: check_assignable(lefthand, parser->tokenizer->messages);
            case RightCompare:
            case RightRange:
            case RightBinary:
                lefthand = parse_binary_operation(lefthand, operator, parser);
                break;

            case RightDeclaration:
                if(!(lefthand->flags & fType)) return lefthand;
                lefthand = parse_declaration(lefthand, next(parser->tokenizer), parser);
                break;

            case RightIndex:
                lefthand = parse_indexing(lefthand, parser);
                break;

            case RightCall:
                lefthand = lefthand->flags & fType
                               ? parse_function_lambda((void*) lefthand, parser)
                               : parse_function_call(lefthand, parser);
                break;

            case RightFieldAccess:
                lefthand = parse_field_access(lefthand, parser);
                break;

            default: unreachable();
        }
    }

    return lefthand;
}
