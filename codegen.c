#include "compiler.h"
#include <stdarg.h>
#include <stdio.h>
#include "helpers/vector.h"

static struct compile_process* current_process = NULL;

void
codegen_new_scope (int flags)
{
    #warning "The resolver needs to exist for this to work."
}

void
codegen_finish_scope ()
{
    #warning "The resolver needs to exist for this to work."
}

struct node*
codegen_node_next ()
{
    return vector_peek_ptr (current_process->node_tree_vec);
}

void
asm_push_args (const char* insn, va_list args)
{
    va_list args2;
    va_copy (args2, args);
    vfprintf (stdout, insn, args);
    fprintf (stdout, "\n");
    if (current_process->ofile)
    {
        vfprintf (current_process->ofile, insn, args2);
        fprintf (current_process->ofile, "\n");
    }
}

void
asm_push (const char* insn, ...)
{
    va_list args;
    va_start (args, insn);
    asm_push_args (insn, args);
    va_end (args);
}

void
codegen_generate_data_section_part (struct node* node)
{
    /* Switch to process the global data..  */

}

void
codegen_generate_data_section ()
{
    asm_push ("section .data");
    struct node* node = NULL;
    while ((node = codegen_node_next ()) != NULL)
    {
        codegen_generate_data_section_part (node);
    }
}

void
codegen_generate_root_node (struct node* node)
{
    /* Process any functions.  */
}

void
codegen_generate_root ()
{
    asm_push ("section .text");
    struct node* node = NULL;
    while ((node = codegen_node_next ()) != NULL)
    {
        codegen_generate_root_node (node);
    }
}

void
codegen_write_strings ()
{
    #warning "Loop through the string table and write all the strings"
}

void
codegen_generate_readonly ()
{
    asm_push ("section .rodata");
    codegen_write_strings ();
}

int
codegen (struct compile_process* process)
{
    current_process = process;
    scope_create_root (process);
    vector_set_peek_pointer (process->node_tree_vec, 0);
    codegen_new_scope (0);
    codegen_generate_data_section ();
    vector_set_peek_pointer (process->node_tree_vec, 0);
    codegen_generate_root ();
    codegen_finish_scope ();

    /* Generate read only data.  */
    codegen_generate_readonly ();
    return 0;
}
