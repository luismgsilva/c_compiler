#include "compiler.h"


/*
 * Format: {operator1, operator2, operatoe3, ..., NULL}
 * Ref: https://en.cppreference.com/w/c/language/operator_precedence
 *
 * When we start making expressions, we will go through this table and if we
 * incorrectly parsed a plus (`+`) when multiplication was priority for example,
 * we will flip the nodes so that they are in the correct order.
 */
struct expressionable_op_precedence_group op_precedence[TOTAL_OPERATOR_GROUPS] = {
    {.operators={"++", "--", "()", "[]", "(", "[", ".", "->", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"*", "/", "%", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"+", "-", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"<<", ">>", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"<", "<=", ">", ">=", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"==", "!=", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"&", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"^", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"|", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"&&", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"||", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT},
    {.operators={"?", ":", NULL}, .associtivity=ASSOCIATIVITY_RIGHT_TO_LEFT},
    {.operators={"+", "+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "^=", "|=", NULL}, .associtivity=ASSOCIATIVITY_RIGHT_TO_LEFT},
    {.operators={",", NULL}, .associtivity=ASSOCIATIVITY_LEFT_TO_RIGHT}
};
