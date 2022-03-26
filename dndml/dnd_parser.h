#ifndef DND_PARSER_H
#define DND_PARSER_H

#include <math.h>
#include "dnd_input_reader.h"
#include "dnd_lexer.h"
#include "dnd_charsheet.h"

enum parser_err { ok = 0, parser_syntax_error, null_ptr_error };

typedef struct token_vec {
  token_t *tokens;
  size_t token_count;
} token_vec_t;

typedef struct parser {
  token_vec_t token_vec;
  lexer_t *lexer;
  char *src_filename;
  enum parser_err (*consume)(struct parser *, enum token_type);
  charsheet_t *(*parse)(struct parser *);
  size_t tok_i; // index of current token
} parser_t;

void construct_parser(parser_t *dest, lexer_t *lex, char *src_filename);

#endif // DND_PARSER_H
