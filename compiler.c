#include "compiler.h"

struct lex_process_functions compiler_lex_functions = {
	.next_char = compile_process_next_char,
	.peek_char = compile_process_peek_char,
	.push_char = compile_process_push_char
};

int
compile_file (const char* file_name, const char* out_file_name, int flags)
{
	struct compile_process* process = compile_process_create(file_name, out_file_name, flags);
	if (!process)
		return COMPILER_FAILED_WITH_ERRORS;

	/* Preform lexical analysis */
	struct lex_process *lex_process = lex_process_create(process, &compiler_lex_functions, NULL);
	/* Preform parsing */

	/* Preform code generation */

	return COMPILER_FILE_COMPILED_OK;
}