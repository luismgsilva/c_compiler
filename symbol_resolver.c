#include "compiler.h"
#include "helpers/vector.h"


static void
symbol_resolver_push_symbol (struct compile_process* process, struct symbol* sym)
{
    vector_push(process->symbols.table, &sym);
}

void
symbol_resolver_initialize (struct compile_process* process)
{
    process->symbols.tables = vector_create(sizeof(struct vector*));


}

void
symbol_resolver_new_table (struct compile_process* process)
{
    /* Save the current table. */
    vector_push(process->symbols.tables, &process->symbols.table);

    /* Override the active table. */
    process->symbols.table = vector_create(sizeof(struct symbol*));
}

void
symbol_resolver_end_table (struct compile_process* process)
{
    struct vector* last_table = vector_back_ptr(process->symbols.tables);
    process->symbols.table = last_table;

    vector_pop(process->symbols.tables);
}

struct symbol*
symbol_resolver_get_symbol (struct compile_process* process, const char* name)
{
    vector_set_peek_pointer(process->symbols.table, 0);
    struct symbol* symbol = vector_peek_ptr(process->symbols.table);
    while (symbol)
    {
        if (S_EQ(symbol->name, name))
        {
            break;
        }

        symbol = vector_peek_ptr(process->symbols.table);
    }
    return symbol;
}

struct symbol*
symbol_resolver_get_symbol_for_native_function (struct compile_process* process, const char* name)
{
    struct symbol* sym = symbol_resolver_get_symbol(process, name);
    if (!sym)
    {
        return NULL;
    }

    if (sym->type != SYMBOL_TYPE_NATIVE_FUNCTION)
    {
        return NULL;
    }

    return sym;
}

struct symbol*
symbol_resolver_register_symbol (struct compile_process* process, const char* symbol_name, int type, void* data)
{
    /* Symbols can never share the same name. */
    if (symbol_resolver_get_symbol(process, symbol_name))
    {
        return NULL;
    }

    struct symbol* sym = calloc(1, sizeof(struct symbol));
    sym->name = symbol_name;
    sym->type = type;
    sym->data = data;
    symbol_resolver_push_symbol(process, sym);
    return sym;
}

struct node*
symbol_resolver_node (struct symbol* sym)
{
    if (sym->type != SYMBOL_TYPE_NODE)
    {
        return NULL;
    }

    return sym->data;
}

void
symbol_resolver_build_for_variable_node (struct compile_process* process, struct node* node)
{
    compiler_error(process, "Variables not yet supported\n");
}

void
symbol_resolver_build_for_function_node (struct compile_process* process, struct node* node)
{
    compiler_error(process, "Functions are not yet supported\n");
}

void
symbol_resolver_build_for_structure_node (struct compile_process* process, struct node* node)
{
    if (node->flags & NODE_FLAG_IS_FORWARD_DECLARATION)
    {
        /* We do not register forward declarations. */
        return;
    }

    symbol_resolver_register_symbol(process, node->_struct.name, SYMBOL_TYPE_NODE, node);
}

void
symbol_resolver_build_for_union_node (struct compile_process* process, struct node* node)
{
    compiler_error(process, "Union are not yet supported\n");
}

void
symbol_resolver_build_for_node (struct compile_process* process, struct node* node)
{
    switch (node->type)
    {
        case NODE_TYPE_VARIABLE:
            symbol_resolver_build_for_variable_node(process, node);
            break;

        case NODE_TYPE_FUNCTION:
            symbol_resolver_build_for_function_node(process, node);
            break;

        case NODE_TYPE_STRUCT:
            symbol_resolver_build_for_structure_node(process, node);
            break;

        case NODE_TYPE_UNION:
            symbol_resolver_build_for_union_node(process, node);
            break;

        /* Ignore all other node types, because they cannot become symbols.*/
    }
}


