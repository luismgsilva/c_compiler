#include "compiler.h"
#include "helpers/vector.h"
#include <assert.h>

static struct compile_process *current_process;
static struct token *parser_last_token;
extern struct expressionable_op_precedence_group op_precedence[TOTAL_OPERATOR_GROUPS];
/* History system */
struct history
{
    int flags;
};

struct history*
history_begin (int flags)
{
    struct history* history = calloc(1, sizeof(struct history));
    history->flags = flags;
    return history;
}

struct history*
history_down (struct history* history, int flags)
{
    struct history* new_history = calloc(1, sizeof(struct history));
    memcpy(new_history, history, sizeof(struct history));
    new_history->flags = flags;
    return new_history;
}

int parse_expressionable_single (struct history* history);
void parse_expressionable (struct history* history);

static void
parser_ignore_nl_or_comment (struct token *token)
{
    while (token && token_is_nl_or_comment_or_newline_seperator(token))
    {
        /* Skip the token */
        vector_peek(current_process->token_vec);

        /*
         * The parser does not care about new lines,
         * only the preprocessor needs to worry about that.
         */
        token = vector_peek_no_increment(current_process->token_vec);
    }
}

/* Grad the next token to process. */
static struct token
*token_next ()
{
    struct token *next_token = vector_peek_no_increment(current_process->token_vec);
    /* Ignore the newline or comment tokens. */
    parser_ignore_nl_or_comment(next_token);
    /*
     * We need to know the line and column we are currently
     * processing to be used in errors messages.
     */
    current_process->pos = next_token->pos;

    parser_last_token = next_token;
    return vector_peek(current_process->token_vec);
}

static struct token*
token_peek_next ()
{
    struct token *next_token = vector_peek_no_increment(current_process->token_vec);
    /* Ignore the newline or comment tokens. */
    parser_ignore_nl_or_comment(next_token);
    return vector_peek_no_increment(current_process->token_vec);
}

void
parse_single_token_to_node ()
{
    struct token *token = token_next();
    struct node *node = NULL;
    switch (token->type)
    {
        case TOKEN_TYPE_NUMBER:
            node = node_create(&(struct node){.type=NODE_TYPE_NUMBER, .llnum=token->llnum});
            break;
        case TOKEN_TYPE_IDENTIFIER:
            node = node_create(&(struct node){.type=TOKEN_TYPE_IDENTIFIER, .sval=token->sval});
            break;
        case TOKEN_TYPE_STRING:
            node = node_create(&(struct node){.type=TOKEN_TYPE_STRING, .sval=token->sval});
            break;
        default:
            compiler_error(current_process, "This is not a single token that can be converted to a node.");
    }
}

void
parse_expressionable_for_op (struct history* history, const char* op)
{
    parse_expressionable(history);
}

static int
parser_get_precedence_for_operator (const char* op, struct expressionable_op_precedence_group** group_out)
{
    *group_out = NULL;
    for (int i = 0; i < TOTAL_OPERATOR_GROUPS; i++)
    {
        for (int j = 0; op_precedence[i].operators[j]; j++)
        {
            const char* _op = op_precedence[i].operators[j];
            if (S_EQ(op, _op))
            {
                *group_out = &op_precedence[i];
                return i;
            }
        }
    }

    return -1;
}

static bool
parser_left_op_has_priority (const char* op_left , const char* op_right)
{
    struct expressionable_op_precedence_group* group_left = NULL;
    struct expressionable_op_precedence_group* group_right = NULL;

    if (S_EQ(op_left, op_right))
    {
        return false;
    }

    int precedence_left = parser_get_precedence_for_operator(op_left, &group_left);
    int precedence_right = parser_get_precedence_for_operator(op_right, &group_right);

    if (group_left->associtivity == ASSOCIATIVITY_RIGHT_TO_LEFT)
    {
        return false;
    }

    return precedence_left <= precedence_right;
}

void
parser_node_shift_children_left (struct node* node)
{
    assert(node->type == NODE_TYPE_EXPRESSION);
    assert(node->exp.right->type == NODE_TYPE_EXPRESSION);

    const char* right_op = node->exp.right->exp.op;
    struct node* new_exp_left_node = node->exp.left;
    struct node* new_exp_right_node = node->exp.right->exp.left;
    make_exp_node(new_exp_left_node, new_exp_right_node, node->exp.op);

    /* (50*20) */
    struct node* new_left_operand = node_pop();
    /* 120 */
    struct node* new_right_operand = node->exp.right->exp.right;

    node->exp.left = new_left_operand;
    node->exp.right = new_right_operand;
    node->exp.op = right_op;
}

void
parser_reorder_expression (struct node** node_out)
{
    struct node * node = *node_out;
    if (node->type != NODE_TYPE_EXPRESSION)
    {
        return;
    }

    /* No expressions, nothing to do... */
    if (node->exp.left->type != NODE_TYPE_EXPRESSION &&
        node->exp.right && node->exp.right->type != NODE_TYPE_EXPRESSION)
    {
        return;
    }

    /*
     * e.i 50*E(30+20)
     *     50*EXPRESSION
     *     EXPRESSION(50*EXPRESSION(30+20))
     *     (50*30) + 20
     */
    if (node->exp.left->type != NODE_TYPE_EXPRESSION &&
        node->exp.right && node->exp.right->type == NODE_TYPE_EXPRESSION)
    {
        const char* right_op = node->exp.right->exp.op;
        if (parser_left_op_has_priority(node->exp.op, right_op))
        {
            /*
            * 50*E(20+120)
            * E(50*20)+120
            */
            parser_node_shift_children_left(node);

            /* We just changed the abstract syntax tree, so we now need to
             * call this reorder function on itself, so that it can do through
             * it again, and reorder if necessary. Because we have made a change
             * that might affect the rest of the tree. */
            parser_reorder_expression(&node->exp.left);
            parser_reorder_expression(&node->exp.right);

        }
    }
}

void
parse_exp_normal (struct history* history)
{
    struct token* op_token = token_peek_next();
    const char* op = op_token->sval;
    struct node* node_left = node_peek_expressionable_or_null();
    /* If the last node is not compatible to an expression, return. */
    if (!node_left)
    {
        return;
    }

    /* Pop off the operator token. */
    token_next();

    /* Pop off the left node. */
    node_pop();

    node_left->flags |= NODE_FLAG_INSIDE_EXPRESSION;

    parse_expressionable_for_op(history_down(history, history->flags), op);
    struct node* node_right = node_pop();
    node_right->flags |= NODE_FLAG_INSIDE_EXPRESSION;

    make_exp_node(node_left, node_right, op);
    struct node* exp_node = node_pop();

    /*
     * Reorder the expression.
     * Multiplication has priority over addition.
     */
    parser_reorder_expression(&exp_node);

    /* Push it back to the stack. */
    node_push(exp_node);

}

/*
 * Responsible for parsing an operator,
 * and merging with the right operand.
 */
int
parse_exp (struct history* history)
{
    parse_exp_normal(history);
    return 0;
}

int
parse_expressionable_single (struct history* history)
{
    struct token* token = token_peek_next();
    if (!token)
    {
        return -1;
    }

    /* When this flag is set, it means that we are inside an expression */
    history->flags |= NODE_FLAG_INSIDE_EXPRESSION;
    int res = -1;
    switch (token->type)
    {
        case TOKEN_TYPE_NUMBER:
            parse_single_token_to_node();
            res = 0;
            break;

        case TOKEN_TYPE_OPERATOR:
            parse_exp(history);
            res = 0;
            break;
    }
    return res;
}

/*
 * Go through all of the tokens, create nodes out of them
 * and try to form an expression from the nodes it created.
 */
void
parse_expressionable (struct history* history)
{
    while(parse_expressionable_single(history) == 0)
    {
    }
}

int
parse_next()
{
    struct token *token = token_peek_next();
    if (!token)
    {
        return -1;
    }

    int res = 0;
    switch (token->type)
    {
        case TOKEN_TYPE_NUMBER:
        case TOKEN_TYPE_IDENTIFIER:
        case TOKEN_TYPE_STRING:
            parse_expressionable(history_begin(0));
            break;
    }
    return 0;
}

int
parse (struct compile_process *process)
{
    current_process = process;
    parser_last_token = NULL;

    node_set_vector(process->node_vec, process->node_tree_vec);

    struct node *node = NULL;
    vector_set_peek_pointer(process->token_vec, 0);
    /* This is the root of the tree. */
    while (parse_next() == 0)
    {
        node = node_peek();
        /* Push to the root of the tree. */
        vector_push(process->node_tree_vec, &node);
    }
    return PARSE_ALL_OK;
}