#ifndef C_COMPILER_H
#define C_COMPILER_H

#include <stdio.h>
#include <stdbool.h>


/*
 * Position of a token with the exact coordinates
 * of it, including in which file it is.
 */
struct pos
{
	int line;
	int col;
	const char filename;
};

/* Token Types */
enum
{
	TOKEN_TYPE_IDENTIFIER,
	TOKEN_TYPE_KEYWORD,
	TOKEN_TYPE_OPERATOR,
	TOKEN_TYPE_SYMBOL,
	TOKEN_TYPE_NUMBER,
	TOKEN_TYPE_STRING,
	TOKEN_TYPE_COMMENT,
	TOKEN_TYPE_NEWLINE
};

struct token
{
	int type;
	int flags;

	/*
	 * Only one of the following datatypes will
	 * be used, so it makes sense to use an union
	 * so that we will only allocate space for
	 * what we need.
	 * */
	union
	{
		char cval; /* Char Value */
		const char *sval; /* String Value */
		unsigned int inum; /* Int Value */
		unsigned long lnum; /* Long Value */
		unsigned long long llnum; /* Long Long Value */
		void* any; /* Points to any of the above datatypes */
	};

	/*
	 * True if there is a whitespace between the token
	 * and the next token.
	 * e.i "* a" - for operator token "+" would mean the
	 * whitespace would be set for token "a".
	 */
	bool whitespace;

	/*
	 * It points for the start of a buffer of content
	 * between brackets for debugging purposes.
	 * e.i (50+10+20)
	 * */
	const char *between_brackets; /* 50+10+20 */
};

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