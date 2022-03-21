#include <stdio.h>
#include <stdlib.h>
#include "dnd_input_reader.h"
#include "dnd_lexer.h"
#include "dnd_charsheet.h"
#include "dnd_parser.h"
#include "../dice.h"

static void add_token_to_vec(token_vec_t *dest, token_t token) {
  dest->token_count++;
  dest->tokens = realloc(
    dest->tokens,
    dest->token_count * sizeof(token_t)
  );
  dest->tokens[dest->token_count - 1] = token;
};

static enum parser_err parser_consume(
  parser_t *this,
  enum token_type type
) {
  if (this == NULL) return fatal_error;
  if (this->token_vec[this->tok_i].type == type) {

  }
}

static charsheet_t *parser_parse(parser_t *this) {
  
}

void construct_parser(parser_t *dest, lexer_t *lex, char *src_filename) {
  dest->lexer = lex;
  dest->src_filename = src_filename;
  dest->consume = &parser_consume;
  dest->parse = &parser_parse;

  dest->token_vec.tokens = NULL;
  dest->token_vec.token_count = 0;
  token_t current_token;
  do {
    current_token = dest->lexer->get_next_token(dest->lexer);
    if (current_token.type == syntax_error) {
      fprintf(
        stderr,
        "Syntax error in token stream generated while parsing.\n"
      );
      dest->token_vec.tokens = NULL;
      dest->token_vec.tokens = 0;
      break;
    }
    add_token_to_vec(dest->token_vec, current_token);
  } while (current_token.type != eof);
  dest->tok_i = 0;
}
