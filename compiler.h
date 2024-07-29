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

#define SYMBOL_CASE \
	case '{':   \
	case '}':   \
	case ':':   \
	case ';':   \
	case '#':   \
	case '\\':  \
	case ')':   \
	case ']'

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

enum
{
	NUMBER_TYPE_NORMAL,
	NUMBER_TYPE_LONG,
	NUMBER_TYPE_FLOAT,
	NUMBER_TYPE_DOUBLE
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

	struct token_number
	{
		int type;
	} num;

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

	/* A vector of tokens from lexical analysis. */
	struct vector *token_vec;

	/* Used for push and pop nodes while parsing process. */
	struct vector *node_vec;
	/* Actual root of the tree. */
	struct vector *node_tree_vec;
	FILE *ofile;
};

enum
{
	PARSE_ALL_OK,
	PARSE_GENERAL_ERROR
};

enum
{
	NODE_TYPE_EXPRESSION,
	NODE_TYPE_EXPRESSION_PARENTHESES,
	NODE_TYPE_NUMBER,
	NODE_TYPE_IDENTIFIER,
	NODE_TYPE_STRING,
	NODE_TYPE_VARIABLE,
	NODE_TYPE_VARIABLE_LIST,
	NODE_TYPE_FUNCTION,
	NODE_TYPE_BODY,

	NODE_TYPE_STATEMENT_RETURN,
	NODE_TYPE_STATEMENT_IF,
	NODE_TYPE_STATEMENT_ELSE,
	NODE_TYPE_STATEMENT_WHILE,
	NODE_TYPE_STATEMENT_DO_WHILE,
	NODE_TYPE_STATEMENT_FOR,
	NODE_TYPE_STATEMENT_BREAK,
	NODE_TYPE_STATEMENT_CONTINUE,
	NODE_TYPE_STATEMENT_SWITCH,
	NODE_TYPE_STATEMENT_CASE,
	NODE_TYPE_STATEMENT_DEFAULT,
	NODE_TYPE_STATEMENT_GOTO,

	NODE_TYPE_UNARY,
	NODE_TYPE_TENARY,
	NODE_TYPE_LABEL,
	NODE_TYPE_STRUCT,
	NODE_TYPE_UNION,
	NODE_TYPE_BRACKET,
	NODE_TYPE_CAST,

	/*
	 * A node that doesnt mean anything - like void.
	 * And can be ignored.
	 */
	NODE_TYPE_BLANK
};

enum
{
	NODE_FLAG_INSIDE_EXPRESSION = 0b00000001
};

struct node
{
	int type;
	int flags;

	struct pos pos;

	struct node_binded
	{
		/* Pointer to body node. */
		struct node *owner;

		/* Pointer to the function this node is in. */
		struct node *function;
	} binded;

	union
	{
		struct exp
		{
			struct node* left;
			struct node* right;
			const char* op;
		} exp;
	};

	union {
		char cval;
		const char *sval;
		unsigned int inum;
		unsigned long lnum;
		unsigned long long llnum;
	};
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
int parse (struct compile_process *process);

/* Builds tokens for the input string. */
struct lex_process* tokens_build_for_string (struct compile_process* compiler, const char* str);

bool token_is_keyword (struct token *token, const char *value);
bool token_is_symbol (struct token *token, char c);
bool token_is_nl_or_comment_or_newline_seperator (struct token *token);

struct node *node_create (struct node *_node);
struct node* make_exp_node (struct node* left_node, struct node* right_node, const char* op);
struct node *node_pop ();
struct node *node_peek ();
struct node
*node_peek_or_null ();
void node_push (struct node *node);
void node_set_vector (struct vector *vec, struct vector *root_vec);

bool node_is_expressionable (struct node* node);
struct node* node_peek_expressionable_or_null ();

#define TOTAL_OPERATOR_GROUPS 14
#define MAX_OPERATOR_IN_GROUP 12

enum
{
    ASSOCIATIVITY_LEFT_TO_RIGHT,
    ASSOCIATIVITY_RIGHT_TO_LEFT
};

struct expressionable_op_precedence_group
{
    char* operators[MAX_OPERATOR_IN_GROUP];
    int associtivity;
};

#endif
