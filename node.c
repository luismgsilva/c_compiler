#include "compiler.h"
#include "helpers/vector.h"
#include <assert.h>

struct vector *node_vector = NULL;
struct vector *node_vector_root = NULL;

struct node* parser_current_body = NULL;
struct node* parser_current_function = NULL;

void
node_set_vector (struct vector *vec, struct vector *root_vec)
{
    node_vector = vec;
    node_vector_root = root_vec;
}

void
node_push (struct node *node)
{
    vector_push(node_vector, &node);
}

struct node
*node_peek_or_null ()
{
    return vector_back_ptr_or_null(node_vector);
}

struct node
*node_peek ()
{
    return *(struct node**)(vector_back(node_vector));
}

struct node
*node_pop ()
{
    /* Get the last node of the vector. */
    struct node *last_node = vector_back_ptr(node_vector);
    /* If the node vector is not empty, get the last node on the root. */
    struct node *last_node_root = vector_empty(node_vector) ? NULL : vector_back_ptr_or_null(node_vector_root);

    /* Pop off the last node of the vector. */
    vector_pop(node_vector);

    if (last_node == last_node_root)
    {
        /*
         * Pop off from the root vector as well.
         * This ensures there are no duplicates.
         */
        vector_pop(node_vector_root);
    }

    return last_node;
}

bool
node_is_expressionable (struct node* node)
{
    return node->type == NODE_TYPE_EXPRESSION               ||
           node->type == NODE_TYPE_EXPRESSION_PARENTHESES   ||
           node->type == NODE_TYPE_UNARY                    ||
           node->type == NODE_TYPE_IDENTIFIER               ||
           node->type == NODE_TYPE_NUMBER;
}

/*
 * Peek to the last node on the stack, if its something we
 * can put in an expression, return it. Otherwise return null.
 * If we peek and its function node or something, we cannot put
 * a function into an expression. So, we return null.
 */
struct node*
node_peek_expressionable_or_null ()
{
    struct node* last_node = node_peek_or_null();
    return node_is_expressionable(last_node) ? last_node : NULL;
}

struct node*
make_exp_node (struct node* left_node, struct node* right_node, const char* op)
{
    assert(left_node);
    assert(right_node);
    node_create(&(struct node){.type=NODE_TYPE_EXPRESSION,.exp.left=left_node,.exp.right=right_node,.exp.op=op});
}

void make_bracket_node (struct node* node)
{
    node_create(&(struct node){.type=NODE_TYPE_BRACKET, .bracket.inner=node});
}

void
make_body_node (struct vector* body_vec, size_t size, bool padded, struct node* largest_var_node)
{
    node_create(&(struct node){
        .type=NODE_TYPE_BODY,
        .body.statements=body_vec,
        .body.size=size,
        .body.padded=padded,
        .body.largest_var_node=largest_var_node
    });
}

void
make_struct_node (const char* name, struct node* body_node)
{
    int flags = 0;
    if (!body_node)
    {
        flags |= NODE_FLAG_IS_FORWARD_DECLARATION;
    }

    node_create(&(struct node){.type=NODE_TYPE_STRUCT,._struct.body_n=body_node,._struct.name=name,.flags=flags});

}

void
make_function_node (struct datatype* ret_type, const char* name,
                    struct vector* arguments, struct node* body_node)
{
    struct node* func_node = node_create (&(struct node)
    {
        .type=NODE_TYPE_FUNCTION,
        .func.name=name,
        .func.args.vector=arguments,
        .func.body_n=body_node,
        .func.rtype=*ret_type,
        .func.args.stack_addition=DATA_SIZE_DDWORD
    });
    return func_node;

    #warning "Dont forget to build the frame elementss"
}

struct node*
node_from_sym (struct symbol* sym)
{
    if (sym->type != SYMBOL_TYPE_NODE)
    {
        return NULL;
    }

    return sym->data;
}

struct node*
node_from_symbol (struct compile_process* current_process, const char* name)
{
    struct symbol* sym = symbol_resolver_get_symbol(current_process, name);
    if (!sym)
    {
        return NULL;
    }

    return node_from_sym(sym);
}

struct node*
struct_node_for_name (struct compile_process* current_process, const char* name)
{
    struct node* node = node_from_symbol(current_process, name);
    if (!node)
    {
        return NULL;
    }

    if (node->type != NODE_TYPE_STRUCT)
    {
        return NULL;
    }

    return node;
}


struct node
*node_create (struct node *_node)
{
    struct node *node = malloc(sizeof(struct node));
    memcpy(node, _node, sizeof(struct node));
    #warning "Should set the binded owner and binded function here."
    node->binded.owner = parser_current_body;
    node->binded.function = parser_current_function;
    node_push(node);
    return node;
}

bool
node_is_struct_or_union_variable (struct node* node)
{
    if (node->type == NODE_TYPE_VARIABLE)
    {
        return false;
    }

    return datatype_is_struct_or_union(&node->var.type);
}

struct node*
variable_node (struct node* node)
{
    struct node* var_node = NULL;
    switch (node->type)
    {
        case NODE_TYPE_VARIABLE:
            var_node = node;
            break;

        case NODE_TYPE_STRUCT:
            var_node = node->_struct.var;
            break;

        case NODE_TYPE_UNION:
            /* var_node = node->_union.var; */
            assert(1 == 0 && "Unions are not yet supported.\n");
            break;
    }

    return var_node;
}

bool
variable_node_is_primitive (struct node* node)
{
    assert(node->type == NODE_TYPE_VARIABLE);
    return datatype_is_primitive(&node->var.type);
}

struct node*
variable_node_or_list (struct node* node)
{
    if (node->type == NODE_TYPE_VARIABLE_LIST)
    {
        return node;
    }

    return variable_node(node);
}