#include "compiler.h"
#include "helpers/vector.h"

static struct lex_process *lex_process;

static char
peekc()
{
    return lex_process->function->peek_char(lex_process);
}

struct token
*read_next_token()
{
    struct token *token = NULL;

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
