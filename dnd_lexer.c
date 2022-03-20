#include <stdio.h>
#include <ctype.h>
#include "dnd_input_reader.h"
#include "dnd_lexer.h"

/**
 * The reserved words that start with '@' shall
 * be referred to by convention as "at_labels".
 */
static void lex_at_label(lexer_t *lex, token_t *dest) {
  
}

/**
 * The reserved words that start with '%' shall be
 * referred to by convention as "type_labels".
 */
static void lex_type_label(lexer_t *lex, token_t *dest) {
  
}

static void lex_identifier(lexer_t *lex, token_t *dest) {
  
}

static void lex_number(lexer_t *lex, token_t *dest) {
  
}

static void lex_string_literal(lexer_t *lex, token_t *dest) {
  
}

static void skip_whitespace(lexer_t *lex) {
  
}

static void skip_comment(lexer_t *lex) {
  
}

/**
 * This function is assigned to be the get_next_token
 * method of lexer_t objects in construct_lexer().
 */
static token_t lexer_get_next_token(lexer_t *this) {
  token_t result;
  result.src_text = this->input_reader->text;
  while (this->input_reader->current_char) {
    if (this->input_reader->current_char == '@') {
      lex_at_label(this, &result);
      return result;
    }

    if (isspace(this->input_reader->current_char)) {
      skip_whitespace(this);
      return this->get_next_token(this);
    }

    if (isdigit(this->input_reader->current_char) ||
        this->input_reader->current_char == '-') {
      lex_number(this, &result);
      return result;
    }

    if (this->input_reader->current_char == '"') {
      lex_string_literal(this, &result);
      return result;
    }

    if (this->input_reader->current_char == '~' &&
        this->input_reader->peek(this->input_reader) == '~') {
      skip_comment(this);
      return this->get_next_token(this);
    }

    if (this->input_reader->current_char == '%') {
      lex_type_label(this, &result);
      return result;
    }

    /* identifiers can start with '_' or a letter
       and can contain any letters, digits, '_', or '-' */
    if (this->input_reader->current_char == '_' ||
        isalpha(this->input_reader->current_char)) {
      lex_identifier(this, &result);
      return result;
    }

    if (this->input_reader->current_char == '[') {
      result.start = this->input_reader->pos;
      result.end = result.start + 1;
      result.type = open_sqr_bracket;
      this->input_reader->advance(this->input_reader);
      return result;
    }

    if (this->input_reader->current_char == ']') {
      result.start = this->input_reader->pos;
      result.end = result.start + 1;
      result.type = close_sqr_bracket;
      this->input_reader->advance(this->input_reader);
      return result;
    }

    if (this->input_reader->current_char == ':') {
      result.start = this->input_reader->pos;
      result.end = result.start + 1;
      result.type = colon;
      this->input_reader->advance(this->input_reader);
      return result;
    }

    if (this->input_reader->current_char == ';') {
      result.start = this->input_reader->pos;
      result.end = result.start + 1;
      result.type = semicolon;
      this->input_reader->advance(this->input_reader);
      return result;
    }

    fprintf(
      stderr,
      "Error while lexing: Syntax error at unexpected character %c\n",
      this->input_reader->current_char
    );
    result.start = this->input_reader->pos;
    result.end = result.start + 1;
    result.type = syntax_error;
    return result;
  }

  result.start = result.end = this->input_reader->pos;
  result.type = eof;
  return result;
}

void construct_lexer(lexer_t *dest, input_reader_t *ir) {
  dest->input_reader = ir; // non-owning; *ir must be initialized already
  dest->get_next_token = &lexer_get_next_token;
}
