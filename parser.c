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

static bool
token_next_is_operator (const char* op)
{
    struct token* token = token_peek_next();
    return token_is_operator(token, op);
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
    struct node* node = *node_out;
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

void
parse_identifier (struct history* history)
{
    assert(token_peek_next()->type == NODE_TYPE_IDENTIFIER);
    parse_single_token_to_node();
}


static bool is_keyword_variable_modifier(const char* val)
{
    return S_EQ(val, "unsigned")    ||
           S_EQ(val, "signed")      ||
           S_EQ(val, "statis" )     ||
           S_EQ(val, "const")       ||
           S_EQ(val, "extern")      ||
           S_EQ(val, "__ignore_typecheck__");
}

void
parse_datatype_modifiers (struct datatype* dtype)
{
    struct token* token = token_peek_next();
    while(token && token->type == TOKEN_TYPE_KEYWORD)
    {
        if (!is_keyword_variable_modifier(token->sval))
        {
            break;
        }

        if (S_EQ(token->sval, "signed"))
        {
            dtype->flags |= DATATYPE_FLAG_IS_SIGNED;
        }
        else if (S_EQ(token->sval, "unsigned"))
        {
            dtype->flags &= ~DATATYPE_FLAG_IS_SIGNED;
        }
        else if (S_EQ(token->sval, "static"))
        {
            dtype->flags |= DATATYPE_FLAG_IS_STATIC;
        }
        else if (S_EQ(token->sval, "const"))
        {
            dtype->flags |= DATATYPE_FLAG_IS_CONST;
        }
        else if (S_EQ(token->sval, "extern"))
        {
            dtype->flags |= DATATYPE_FLAG_IS_EXTERN;
        }
        else if (S_EQ(token->sval, "restrict"))
        {
            dtype->flags |= DATATYPE_FLAG_IS_RESTRICT;
        }
        else if (S_EQ(token->sval, "__ignore_typecheck__"))
        {
            dtype->flags |= DATATYPE_FLAG_IS_IGNORE_TYPE_CHECKING;
        }

        token_next();
        token = token_peek_next();
    }
}

void
parser_get_datatype_tokens (struct token** datatype_token, struct token** datatype_secondary_token)
{
    *datatype_token = token_next();
    struct token* next_token = token_peek_next();
    if (token_is_primitive_keywords(next_token))
    {
        *datatype_secondary_token = next_token;
        token_next();
    }
}

int
parser_datatype_expected_for_type_string(const char* str)
{
    int type = DATA_TYPE_EXPECT_PRIMITIVE;
    if (S_EQ(str, "union"))
    {
        type = DATA_TYPE_EXPECT_UNION;
    }
    else if (S_EQ(str, "struct"))
    {
        type = DATA_TYPE_EXPECT_STRUCT;
    }

    return type;
}

int
parser_get_pointer_depth()
{
    int depth = 0;
    while (token_next_is_operator("*"))
    {
        depth++;
        token_next();
    }
    return depth;
}

void parser_datatype_init_type_and_size_for_primitive (struct token* datatype_token, struct token* datatype_secondary_token, struct datatype* datatype_out);
void
parser_datatype_adjust_size_for_secondary (struct datatype* datatype, struct token* datatype_secondary_token)
{
    if (!datatype_secondary_token)
    {
        return;
    }

    struct datatype* secondary_data_type = calloc(1, sizeof(struct datatype));
    parser_datatype_init_type_and_size_for_primitive(datatype_secondary_token, NULL, secondary_data_type);
    datatype->size += secondary_data_type->size;
    datatype->secondary = secondary_data_type;
    datatype->flags |= DATATYPE_FLAG_IS_SECONDARY;
}

bool
parser_datatype_is_secondary_allowed_for_type (const char* type)
{
    /* e.i `long long` is allowed. */
    return S_EQ(type, "long")   ||
           S_EQ(type, "short")  ||
           S_EQ(type, "double") ||
           S_EQ(type, "float");
}

void
parser_datatype_init_type_and_size_for_primitive (struct token* datatype_token, struct token* datatype_secondary_token, struct datatype* datatype_out)
{
    if (parser_datatype_is_secondary_allowed_for_type(datatype_token->sval), datatype_secondary_token)
    {
        compiler_error(current_process, "You are not allowed a secondary datatype here for the given datatype.");
    }

    if (S_EQ(datatype_token->sval, "void"))
    {
        datatype_out->type = DATA_TYPE_VOID;
        datatype_out->size = DATA_SIZE_ZERO;
    }
    else if (S_EQ(datatype_token->sval, "char"))
    {
        datatype_out->type = DATA_TYPE_CHAR;
        datatype_out->size = DATA_SIZE_BYTE;
    }
    else if (S_EQ(datatype_token->sval, "short"))
    {
        datatype_out->type = DATA_TYPE_SHORT;
        datatype_out->size = DATA_SIZE_WORD;
    }
    else if (S_EQ(datatype_token->sval, "int"))
    {
        datatype_out->type = DATA_TYPE_INTEGER;
        datatype_out->size = DATA_SIZE_DWORD;
    }
    else if (S_EQ(datatype_token->sval, "long"))
    {
        datatype_out->type = DATA_TYPE_LONG;
        /* This is to be cahnged later. For now, keep as DWORD. */
        datatype_out->size = DATA_SIZE_DWORD;
    }
    else if (S_EQ(datatype_token->sval, "float"))
    {
        datatype_out->type = DATA_TYPE_FLOAT;
        datatype_out->size = DATA_SIZE_DWORD;
    }
    else if (S_EQ(datatype_token->sval, "double"))
    {
        datatype_out->type = DATA_TYPE_DOUBLE;
        datatype_out->size = DATA_SIZE_DWORD;
    }
    else
    {
        compiler_error(current_process, "BUG: Invalid primitve datatype.\n");
    }

    /*
     * Adjust the size of the datatype according to the secondary one (if any).
     * e.i if `long long` is provided, it will adjust the size from 4 to 8.
     */
    parser_datatype_adjust_size_for_secondary(datatype_out, datatype_secondary_token);
}

bool
parser_datatype_is_secondary_allowed (int expected_type)
{
    return expected_type == DATA_TYPE_EXPECT_PRIMITIVE;
}

void
parser_datatype_init_type_and_size (struct token* datatype_token,
                                struct token* datatype_secondary_token,
                                struct datatype* datatype_out,
                                int pointer_depth, int expected_type)
{
    /*
     * Need to validate the `datatye_secondary_token` becasue,
     * for example, `struct int` is not allowed.
     */
    if (!parser_datatype_is_secondary_allowed(expected_type) && datatype_secondary_token)
    {
        compiler_error(current_process, "You provided an invalid secondary datatype.\n");
    }

    switch (expected_type)
    {
        case DATA_TYPE_EXPECT_PRIMITIVE:
            parser_datatype_init_type_and_size_for_primitive(datatype_token, datatype_secondary_token, datatype_out);
        break;
        case DATA_TYPE_EXPECT_STRUCT:
        case DATA_TYPE_EXPECT_UNION:
            compiler_error(current_process, "Structure and union types are currently unsupported (yet).\n");
        break;

        default:
            compiler_error(current_process, "BUG: Unsupported datatype expectation.\n");
    }
}

void
parser_datatype_init (struct token* datatype_token,
                    struct token* datatype_secondary_token,
                    struct datatype* datatype_out,
                    int pointer_depth, int expected_type)
{
    parser_datatype_init_type_and_size(datatype_token, datatype_secondary_token, datatype_out, pointer_depth, expected_type);
    datatype_out->type_str = datatype_token->sval;

    if (S_EQ(datatype_token->sval, "long") && datatype_secondary_token && S_EQ(datatype_secondary_token->sval, "long"))
    {
        compiler_warning(current_process, "Our compiler does not support 64 bit longs, therefore your `long long` is defaulting to 32 bits.\n");
        datatype_out->size = DATA_SIZE_DWORD;
    }
}

int
parser_get_random_type_index ()
{
    static int x = 0;
    x++;
    return x;
}

struct token*
parser_build_random_type_name()
{
    char tmp_name[25];
    sprintf(tmp_name, "customtypename_%i", parser_get_random_type_index());
    char* sval = malloc(sizeof(tmp_name));
    strncpy(sval, tmp_name, sizeof(tmp_name));

    struct token* token = calloc(1, sizeof(struct token));
    token->type = TOKEN_TYPE_IDENTIFIER;
    token->sval = sval;
    return token;
}

/* e.i `struct love_tania;` */
void
parse_datatype_type (struct datatype* dtype)
{
    struct token* datatype_token = NULL;
    struct token* datatype_secondary_token = NULL;

    /*
     * Define the datatype_token as `struct`.
     * Define the datatype_secondary_token as `NULL`.
     */
    parser_get_datatype_tokens(&datatype_token, &datatype_secondary_token);

    /* Define the `expected_type` as `DATA_TYPE_EXPECTED_STRUCT`. */
    int expected_type = parser_datatype_expected_for_type_string(datatype_token->sval);

    if (datatype_is_struct_or_union_for_name(datatype_token->sval))
    {
        /* Here we parse the name "love_tania". */
        if (token_peek_next()->type == TOKEN_TYPE_IDENTIFIER)
        {
            datatype_token = token_next();
        }
        else
        {
            /*
            * Not all structs have names.
            *  e.i `struct { int x; } love_tania;`
            */
            datatype_token = parser_build_random_type_name();
            dtype->flags |= DATATYPE_FLAG_STRUCT_UNION_NO_NAME;
        }
    }

    /* e.i `int**` */
    int pointer_depth = parser_get_pointer_depth();
    parser_datatype_init(datatype_token, datatype_secondary_token, dtype, pointer_depth, expected_type);
}

void
parse_datatype (struct datatype* dtype)
{
    memset(dtype, 0, sizeof(struct datatype));
    dtype->flags |= DATATYPE_FLAG_IS_SIGNED;

    parse_datatype_modifiers(dtype);
    parse_datatype_type(dtype);
    parse_datatype_modifiers(dtype);
}

void
parse_variable_function_or_struct_union (struct history* history)
{
    struct datatype dtype;
    parse_datatype(&dtype);
}

/* Responsible for parsing all keyword tokens. */
void
parse_keyword (struct history* history)
{
    struct token* token = token_peek_next();
    /* Either parsing a variable, a function, a struct, or a union */
    if (is_keyword_variable_modifier(token->sval) || keyword_is_datatype(token->sval))
    {
        parse_variable_function_or_struct_union(history);
        return;
    }
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

        case TOKEN_TYPE_IDENTIFIER:
            parse_identifier(history);
            res = 0;
            break;

        case TOKEN_TYPE_OPERATOR:
            parse_exp(history);
            res = 0;
            break;
        case TOKEN_TYPE_KEYWORD:
            parse_keyword(history);
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

void
parse_keyword_for_global()
{
    parse_keyword(history_begin(0));
    struct node* node = node_pop();
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
        case TOKEN_TYPE_KEYWORD:
            parse_keyword_for_global();
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