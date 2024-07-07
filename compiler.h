#ifndef C_COMPILER_H
#define C_COMPILER_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define S_EQ(str, str2) \
		(str && str2 && (strcmp(str, str2) == 0))

/*
 * Position of a token with the exact coordinates
 * of it, including in which file it is.
 */
struct pos
{
	int line;
	int col;
	const char *filename;
};

#define NUMERIC_CASE	\
	case '0':	\
	case '1':	\
	case '2':	\
	case '3':	\
	case '4':	\
	case '5':	\
	case '6':	\
	case '7':	\
	case '8':	\
	case '9'

#define OPERATOR_CASE_EXCLUDING_DIVISION \
	case '+':	\
	case '-':	\
	case '*':	\
	case '>':	\
	case '<':	\
	case '^':	\
	case '%':	\
	case '!':	\
	case '=':	\
	case '~':	\
	case '|':	\
	case '&':	\
	case '(':	\
	case '[':	\
	case ',':	\
	case '.':	\
	case '?'

enum
{
	LEXICAL_ANALYSIS_ALL_OK,
	LEXICAL_ANALYSIS_INPUT_ERROR
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
	struct pos pos;

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

struct lex_process;
typedef char (*LEX_PROCESS_NEXT_CHAR)(struct lex_process *process);
typedef char (*LEX_PROCESS_PEEK_CHAR)(struct lex_process *process);
typedef void (*LEX_PROCESS_PUSH_CHAR)(struct lex_process *process, char c);
struct lex_process_functions
{
	LEX_PROCESS_NEXT_CHAR next_char;
	LEX_PROCESS_PEEK_CHAR peek_char;
	LEX_PROCESS_PUSH_CHAR push_char;
};

struct lex_process
{
	struct pos pos;
	struct vector *token_vec;
	struct compile_process *compiler;

	/*
	 * The number of brackets, for example:
	 * ((50)) would result in:
	 * current_expression_count = 2
	 */
	int current_expression_count;
	struct buffer *parentheses_buffer;
	struct lex_process_functions *function;

	/*
	 * This is be private data that the lexer does not
	 * understand, but the person using the lexer does
	 * understand.
	 */
	void *private;
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

	struct pos pos;
	struct compile_process_input_file
	{
		FILE *fp;
		const char *abs_path;
	} cfile;

	FILE *ofile;
};

int compile_file (const char* file_name, const char* out_file_name, int flags);
struct compile_process *compile_process_create (const char* file_name, const char* out_file_name, int flags);

char compile_process_next_char (struct lex_process *lex_process);
char compile_process_peek_char (struct lex_process *lex_process);
void compile_process_push_char (struct lex_process *lex_process, char c);

void compiler_error (struct compile_process *compiler, const char *msg, ...);
void compiler_warning (struct compile_process *compiler, const char *msg, ...);

struct lex_process *lex_process_create (struct compile_process *compiler, struct lex_process_functions *functions, void *private);
void lex_process_free (struct lex_process *process);
void *lex_process_private (struct lex_process *process);
struct vector *lex_process_tokens (struct lex_process *process);
int lex (struct lex_process *process);

bool token_is_keyword (struct token *token, const char *value);

#endif
