#include <stdio.h>
#include "compiler.h"

int
main (void)
{
	int res = compile_file("./test.c", "./test", 0);
	if (res == COMPILER_FILE_COMPILED_OK)
		printf("Everthing looks good.\n");
	else if (res == COMPILER_FAILED_WITH_ERRORS)
		printf("Compilation failed.\n");
	else
		printf("Unknown result for compile file\n");
	return 0;
}
