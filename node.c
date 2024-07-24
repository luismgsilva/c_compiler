#include "compiler.h"
#include "helpers/vector.h"
#include <assert.h>

struct vector *node_vector = NULL;
struct vector *node_vector_root = NULL;

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
    struct node *last_node_root = vector_empty(node_vector) ? NULL : vector_back_ptr(node_vector_root);

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