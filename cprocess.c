#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"
#include "helpers/vector.h"

struct compile_process
*compile_process_create (const char* file_name, const char* out_file_name, int flags)
{
	FILE* file = fopen(file_name, "r");
	if (!file)
	{
		return NULL;
	}

	FILE* out_file = NULL;
	if (out_file_name)
	{
		out_file = fopen(out_file_name, "w");
		if (!out_file)
		{
			return NULL;
		}
	}

	struct compile_process* process = calloc(1, sizeof(struct compile_process));
	process->node_vec = vector_create(sizeof(struct node*));
	process->node_tree_vec = vector_create(sizeof(struct node*));

	process->flags=flags;
	process->cfile.fp = file;
	process->ofile = out_file;
	process->generator = codegenerator_new (process);

	symbol_resolver_initialize(process);
	symbol_resolver_new_table(process);

	return process;
}

char
compile_process_next_char (struct lex_process *lex_process)
{
	struct compile_process *compiler = lex_process->compiler;
	compiler->pos.col += 1;
	char c = getc(compiler->cfile.fp);
	/*
	 * When a newline is found, the position should be reset
	 * to the next line at the starting colomun.
	 */
	if (c == '\n')
	{
		compiler->pos.line += 1;
		compiler->pos.col = 1;
	}

	return c;
}

char
compile_process_peek_char (struct lex_process *lex_process)
{
	struct compile_process *compiler = lex_process->compiler;
	char c = getc(compiler->cfile.fp);

	/* It will push the character back to the file stream. */
	ungetc(c, compiler->cfile.fp);

	return c;
}

void
compile_process_push_char (struct lex_process *lex_process, char c)
{
	struct compile_process *compiler = lex_process->compiler;
	ungetc(c, compiler->cfile.fp);
}
