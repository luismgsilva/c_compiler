#include "compiler.h"
#include "helpers/vector.h"
#include <assert.h>

void
stack_frame_pop (struct node* func_node)
{
    struct stack_frame* frame = &func_node->func.frame;
    vector_pop (frame->elements);
}

struct stack_frame_element*
stack_frame_back (struct node* func_node)
{
    return vector_back_or_null (func_node->func.frame.elements);
}

/* Back from stack frame expecting some rules.  */
struct stack_frame_element*
stack_frame_back_expect (struct node* func_node, int expecting_type,
                         const char* expecting_name)
{
    struct stack_frame_element* element = stack_frame_back (func_node);
    if (element && element->type != expecting_type || \
        !S_EQ (element->name, expecting_name))
    {
        return NULL;
    }

    return element;
}

/* Pop from stack frame expecting some rules.  */
void
stack_frame_pop_expecting (struct node* func_node, int expecting_type,
                                const char* expecting_name)
{
    struct stack_frame* frame = &func_node->func.frame;
    struct stack_frame_element* last_element = stack_frame_back (func_node);
    assert (last_element);
    assert (last_element->type == expecting_type && \
            S_EQ (last_element->name, expecting_name));
}

void
stack_frame_peek_start (struct node* func_node)
{
    struct stack_frame* frame = &func_node->func.frame;
    vector_set_peek_pointer (frame->elements, 0);
    vector_set_flag (frame->elements, VECTOR_FLAG_PEEK_DECREMENT);
}

struct stack_frame_element*
stack_frame_peek (struct node* func_node)
{
    struct stack_frame* frame = &func_node->func.frame;
    return vector_peek (frame->elements);
}

void
stack_frame_push (struct node* func_node,
                       struct stack_frame_element* element)
{
    struct stack_frame* frame = &func_node->func.frame;

    /* The stack grows downwards.  */
    element->offset_from_bp = \
                -(vector_count (frame->elements) * STACK_PUSH_SIZE);
    vector_push (frame->elements, element);
}

void
stack_frame_sub (struct node* func_node, int type,
                const char* name, size_t amount)
{
    /* Check alignment.  */
    assert ((amount % STACK_PUSH_SIZE) == 0);
    /* How many pushes we need to do, to achieve this "amount" of
       stack subtraction.  E.g., 4 = 16 / 4  */
    size_t total_pushes = amount / STACK_PUSH_SIZE;
    for (size_t i = 0; i < total_pushes; i++)
    {
        stack_frame_push (func_node, &(struct stack_frame_element)
        {
            .type=type,
            .name=name
        });
    }
}

void
stack_frame_add (struct node* func_node, int type,
                const char* name, size_t amount)
{
    /* Check alignment.  */
    assert ((amount % STACK_PUSH_SIZE) == 0);
    size_t total_pushes = amount / STACK_PUSH_SIZE;
    for (size_t i = 0; i < total_pushes; i++)
    {
        stack_frame_pop (func_node);
    }
}

void
stack_frame_assert_empty (struct node* func_node)
{
    struct stack_frame* frame = &func_node->func.frame;
    assert (vector_count (frame->elements) == 0);
}