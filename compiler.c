#include "compiler.h"
int
compile_file (const char* file_name, const char* out_file_name, int flags)
{
	struct compile_process* process = compile_process_create(file_name, out_file_name, flags);
	if (!process)
		return COMPILER_FAILED_WITH_ERRORS;

	/* Preform lexical analysis */

	/* Preform parsing */

	/* Preform code generation */

	return COMPILER_FILE_COMPILED_OK;
}