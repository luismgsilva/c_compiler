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

struct scope
{
    int flags;

    /* Pointer of void* */
    struct vector* entities;

    /*
     * Total number of bytes this scope uses.
     * Aligned to 16 bytes.
     */
    size_t size;

    /* NULL if no parent. */
    struct scope* parent;
};

enum
{
	SYMBOL_TYPE_NODE,
	/* Native function is a macro function that only exists in our binary. */
	SYMBOL_TYPE_NATIVE_FUNCTION,
	SYMBOL_TYPE_UNKNOWN
};

struct symbol
{
	/* All symbols need a unique name. They cannot share names. */
	const char* name;

	int type;
	void* data;
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

	struct
	{
		struct scope* root;
		struct scope* current;
	} scope;

	struct
	{
		/*
		 * Current active symbol table.
		 * Hold struct symbol pointers.
		 */
		struct vector* table;

		/* Multiple symbol tables stored in here.. */
		struct vector* tables;
	} symbols;
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

struct array_brackets
{
	/* Vector of struct node*
	   Will contain each part of the array.
	   e.g `int x[50][20];` n_brackets = 2 */
	struct vector* n_brackets;
};

struct node;

struct datatype
{
	int flags;
	/* i.e DATA_TYPE_LONG, DATA_TYPE_INT, DATA_TYPE_FLOAT, etc.. */
	int type;

	/* i.e long int. 'int' being the secondary. */
	struct datatype* secondary;

	/* "long" or "int" or "float" ... */
	const char* type_str;

	/* The sizeof the datatype. */
	size_t size;

	int pointer_depth;

	union
	{
		struct node* struct_node;
		struct node* union_node;
	};

	struct array
	{
		struct array_brackets* brackets;

		/* Sizeof the datatype multiply by the number of indexes. */
		size_t size;
	} array;
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

		struct var
		{
			struct datatype type;
			int padding;
			/* Aligned offset. */
			int aoffset;
			const char* name;
			struct node* val;
		} var;

		struct varlist
		{
			/* A list of struct node* variables. */
			struct vector* list;
		} var_list;

		struct bracket
		{
			/* `int x[50];` `[50]` would be our bracket node.
			   The inner would be NODE_TYPE_NUMBER value of 50. */
			struct node* inner;
		} bracket;

		struct _struct
		{
			const char* name;
			struct node* body_n;
			/* struct abc
			   { } var_name;
			   NULL if not variable attached to structure. */
			struct node* var;
		} _struct;


		struct body
		{
			/* struct node* vector of statements.
			   e.g `int x = 50;` stored as one element in the `statements`. */
			struct vector* statements;

			/* The size of combined variables inside this body. */
			size_t size;

			/* True if the variables size had to be
			   increased due to padding in the body. */
			bool padded;

			/* Pointer to the largest (in size) variable
			   node in the statement vector.*/
			struct node* largest_var_node;
		} body;
	};

	union {
		char cval;
		const char *sval;
		unsigned int inum;
		unsigned long lnum;
		unsigned long long llnum;
	};
};

enum
{
	DATATYPE_FLAG_IS_SIGNED  		= 0b00000000001,
	DATATYPE_FLAG_IS_STATIC  		= 0b00000000010,
	DATATYPE_FLAG_IS_CONST   		= 0b00000000100,
	DATATYPE_FLAG_IS_POINTER 		= 0b00000001000,
	DATATYPE_FLAG_IS_ARRAY   		= 0b00000010000,
	DATATYPE_FLAG_IS_EXTERN   		= 0b00000100000,
	DATATYPE_FLAG_IS_RESTRICT 		= 0b00001000000,
	DATATYPE_FLAG_IS_IGNORE_TYPE_CHECKING	= 0b00010000000,
	DATATYPE_FLAG_IS_SECONDARY 		= 0b00100000000,
	DATATYPE_FLAG_STRUCT_UNION_NO_NAME 	= 0b01000000000,
	DATATYPE_FLAG_IS_LITERAL 		= 0b10000000000
};

enum
{
	DATA_TYPE_VOID,
	DATA_TYPE_CHAR,
	DATA_TYPE_SHORT,
	DATA_TYPE_INTEGER,
	DATA_TYPE_LONG,
	DATA_TYPE_FLOAT,
	DATA_TYPE_DOUBLE,
	DATA_TYPE_STRUCT,
	DATA_TYPE_UNION,
	DATA_TYPE_UNKNOWN
};

enum
{
	DATA_TYPE_EXPECT_PRIMITIVE,
	DATA_TYPE_EXPECT_UNION,
	DATA_TYPE_EXPECT_STRUCT
};

enum
{
	DATA_SIZE_ZERO = 0,
	DATA_SIZE_BYTE = 1,
	DATA_SIZE_WORD = 2,
	DATA_SIZE_DWORD = 4,
	DATA_SIZE_DDWORD = 8
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
bool keyword_is_datatype (const char* str);
bool token_is_primitive_keywords (struct token* token);
bool token_is_operator (struct token* token, const char* val);

bool datatype_is_struct_or_union_for_name (const char* name);
size_t datatype_size_for_array_access (struct datatype* dtype);
size_t datatype_element_size (struct datatype* dtype);
size_t datatype_size_no_ptr (struct datatype* dtype);
size_t datatype_size (struct datatype* dtype);
bool datatype_is_primitive (struct datatype* dtype);

struct node *node_create (struct node *_node);
struct node* make_exp_node (struct node* left_node, struct node* right_node, const char* op);
void make_bracket_node (struct node* node);
void make_body_node (struct vector* body_vec, size_t size, bool padded, struct node* largest_var_node);

struct node *node_pop ();
struct node *node_peek ();
struct node
*node_peek_or_null ();
void node_push (struct node *node);
void node_set_vector (struct vector *vec, struct vector *root_vec);

bool node_is_expressionable (struct node* node);
struct node* node_peek_expressionable_or_null ();
bool node_is_struct_or_union_variable (struct node* node);

struct array_brackets* array_brackets_new ();
void array_brackets_free (struct array_brackets* brackets);
void array_brackets_add (struct array_brackets* brackets, struct node* bracket_node);
struct vector* array_brackets_node_vector (struct array_brackets* brackets);
size_t array_brackets_calculate_size_from_index (struct datatype* dtype, struct array_brackets* brackets, int index);
size_t array_brackets_calculate_size (struct datatype* dtype, struct array_brackets* brackets);
int array_total_indexes (struct datatype* dtype);
bool datatype_is_struct_or_union (struct datatype* dtype);
struct node* variable_struct_or_union_body_node(struct node* node);
struct node* variable_node_or_list (struct node* node);

/* Gets the variable size from the given variable node. */
size_t variable_size (struct node* var_node);
/* Sums the variable size of all variable nodes inside the variable list node. */
size_t variable_size_for_list (struct node* var_list_node);
struct node* variable_node (struct node* node);
bool variable_node_is_primitive (struct node* node);

int padding (int val, int to);
int align_value (int val, int to);
int align_value_treat_positive (int val, int to);
int compute_sum_padding (struct vector* vec);

struct scope* scope_new(struct compile_process *process, int flags);
struct scope* scope_create_root (struct compile_process* process);
void scope_free_root (struct compile_process* process);
void scope_iteration_start (struct scope* scope);
void scope_iteration_end (struct scope* scope);
void* scope_iterate_back (struct scope* scope);
void* scope_last_entity_at_scope (struct scope* scope);
void* scope_last_entity_from_scope_stop_at (struct scope* scope, struct scope* stop_scope);
void* scope_last_entity_stop_at (struct compile_process* process, struct scope* stop_scope);
void* scope_last_entity (struct compile_process* process);
void scope_push (struct compile_process* process, void* ptr, size_t elem_size);
void scope_finish (struct compile_process* process);
struct scope* scope_current (struct compile_process* process);

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
