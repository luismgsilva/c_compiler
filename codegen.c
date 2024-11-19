#include "compiler.h"
#include <stdarg.h>
#include <stdio.h>

static struct compile_process* current_process = NULL;

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

int
codegen (struct compile_process* process)
{
    current_process = process;
    asm_push ("jmp %s", "label_name");
    return 0;
}
