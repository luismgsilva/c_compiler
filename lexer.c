#include "compiler.h"
#include <string.h>
#include "helpers/vector.h"
#include "helpers/buffer.h"
#include <assert.h>

#define LEX_GETC_IF(buffer, c, exp) 	\
    for (c =peekc(); exp; c = peekc(c))	\
    {					\
        buffer_write(buffer, c); 	\
        nextc();			\
    }

struct token *read_next_token ();
static struct lex_process *lex_process;
static struct token tmp_token;

static char
peekc()
{
    return lex_process->function->peek_char(lex_process);
}

static char
nextc ()
{
    char c = lex_process->function->next_char(lex_process);
    lex_process->pos.col +=1;
    if (c == '\n')
    {
        lex_process->pos.line += 1;
        lex_process->pos.col = 1;
    }

    return c;
}

static void
pushc (char c)
{
    lex_process->function->push_char(lex_process, c);
}

static struct pos
lex_file_position ()
{
    return lex_process->pos;
}

struct token
*token_create (struct token *_token)
{
    memcpy(&tmp_token, _token, sizeof(struct token));
    tmp_token.pos = lex_file_position();
    return &tmp_token;
}

static struct token
*lexer_last_token ()
{
    return vector_back_or_null(lex_process->token_vec);
}

static struct token
*handler_whitespace ()
{
    struct token *last_token = lexer_last_token();
    if (last_token)
    {
        last_token->whitespace = true;
    }

    nextc();
    return read_next_token();
}

const char
*read_number_str ()
{
    const char *num = NULL;
    struct buffer *buffer = buffer_create();
    char c = peekc();
    /* Extract all the digits from 0 to 9 from a number.  */
    LEX_GETC_IF(buffer, c, (c >= '0' && c <= '9'));

    /* Write NULL terminator to the string (0xx0) */
    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}

unsigned long long
read_number()
{
    const char *s = read_number_str();
    /* Convert string to a long long integer and return it. */
    return atoll(s);
}

struct token
*token_make_number_for_value (unsigned long number)
{
    return token_create(&(struct token){.type=TOKEN_TYPE_NUMBER,.llnum=number});
}

struct token
*token_make_number ()
{
    return token_make_number_for_value(read_number());
}

struct token
*token_make_string (char start_delim, char end_delim)
{
    /*
     * What is a start_delim and end_delim?
     * In a "Hello World" string, the start_delim is
     * the first double quote '"' and the end_delim is
     * the second double quote '""
     */

    struct buffer *buf = buffer_create();
    assert(nextc() == start_delim);
    char c = nextc();
    for (; c != end_delim && c != EOF; c = nextc())
    {
        if (c == '\\')
        {
            /*
             * TODO: Handle backspace character
             * "Hello World\n" <- "\"
             */
            continue;
        }

        buffer_write(buf, c);
    }

    /* Write NULL terminator to the string (0xx0) */
    buffer_write(buf, 0x00);
    return token_create(&(struct token){TOKEN_TYPE_STRING,.sval=buffer_ptr(buf)});
}

struct token
*read_next_token()
{
    struct token *token = NULL;
    char c = peekc();
    switch (c)
    {
        NUMERIC_CASE:
            token = token_make_number();
            break;

        case '"':
            token = token_make_string('"', '"');
            break;

        /* Ignore whitespaces */
        case ' ':
        case '\t':
            token = handler_whitespace();
            break;

        case EOF:
            /* Finished lexical analysis on the file. */
            break;
        default:
            compiler_error(lex_process->compiler, "unexpected token\n");
    }
    return token;
}

int
lex (struct lex_process *process)
{
    process->current_expression_count = 0;
    process->parentheses_buffer = NULL;
    lex_process = process;
    process->pos.filename = process->compiler->cfile.abs_path;

    struct token *token = read_next_token();
    while (token)
    {
        vector_push(process->token_vec, token);
        token = read_next_token();
    }

    return LEXICAL_ANALYSIS_ALL_OK;
}
