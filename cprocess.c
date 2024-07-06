#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"
struct compile_process *compiler_process_create(const char* file_name, const char* out_file_name, int flags)
{
	FILE* file = fopen(file_name, "r");
	if (!file)
	{
		return NULL;
	}

	FILE* out_file = NULL;
	if (out_file_name)
	{
		FILE* out_file = fopen(out_file_name, "w");
		if (!out_file)
		{
			return NULL;
		}
	}

	struct compile_process* process = calloc(1, sizeof(struct compile_process));
	process->flags=flags;
	process->cfile.fp = file;
	process->ofile = out_file;

	return process;
}