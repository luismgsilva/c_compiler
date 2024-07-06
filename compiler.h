#ifndef C_COMPILER_H
#define C_COMPILER_H

#include <stdio.h>

enum
{
	COMPILER_FILE_COMPILED_OK,
	COMPILER_FAILED_WITH_ERRORS,
};

struct compile_process
{
	/* Flags on how this file should be compiled. */
	int flags;

	struct compile_process_input_file
	{
		FILE *fp;
		const char *abs_path;
	} cfile;

	FILE *ofile;
};

int compile_file (const char* file_name, const char* out_file_name, int flags);
struct compile_process *compile_process_create (const char* file_name, const char* out_file_name, int flags);

#endif