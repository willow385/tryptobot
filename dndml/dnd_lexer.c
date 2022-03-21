#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "dnd_input_reader.h"
#include "dnd_lexer.h"

const char *reserved_words[] = {
  "@section",
  "@end-section",
  "@field",
  "%stat",
  "%string",
  "%int",
  "%dice",
  "%deathsaves",
  "%itemlist",
  "%item",
  "NULL"
};

size_t reserved_word_count = 11;

static inline void lex_at_label_or_type_label(lexer_t *lex, token_t *dest) {
  dest->start = lex->input_reader->pos;
  lex->input_reader->advance(lex->input_reader); // skip the '%' or '@'
  while (isalpha(lex->input_reader->current_char) ||
         lex->input_reader->current_char == '-'   ||
         lex->input_reader->current_char == '_') {
    lex->input_reader->advance(lex->input_reader);
  }
  dest->end = lex->input_reader->pos;

  /* If the proceeding for-loop doesn't assign a token_type to dest->type,
     it will have the syntax_error type by default, since any type_label
     not in reserved_words is in fact a syntax error. */
  dest->type = syntax_error;
  for (enum token_type i = 0; i < reserved_word_count; i++) {
    if (!strncmp(
      dest->src_text + dest->start,
      reserved_words[i],
      dest->end - dest->start
    )) {
      dest->type = i;
      break;
    }
  }
}

/**
 * The reserved words that start with '@' shall
 * be referred to by convention as "at_labels".
 */
static void lex_at_label(lexer_t *lex, token_t *dest) {
  lex_at_label_or_type_label(lex, dest);
}

/**
 * The reserved words that start with '%' shall be
 * referred to by convention as "type_labels".
 */
static void lex_type_label(lexer_t *lex, token_t *dest) {
  lex_at_label_or_type_label(lex, dest);
}

static void lex_identifier(lexer_t *lex, token_t *dest) {
  dest->start = lex->input_reader->pos;
  while (isalpha(lex->input_reader->current_char) ||
         lex->input_reader->current_char == '-'   ||
         lex->input_reader->current_char == '_') {
    lex->input_reader->advance(lex->input_reader);
  }
  dest->end = lex->input_reader->pos;
  if (!strncmp(
    dest->src_text + dest->start,
    reserved_words[null_val],
    dest->end - dest->start
  )) {
    dest->type = null_val;
  } else {
    dest->type = identifier;
  }
}

static void lex_number(lexer_t *lex, token_t *dest) {
  dest->type = int_literal;
  dest->start = lex->input_reader->pos;
  while (isdigit(lex->input_reader->peek(lex->input_reader))) {
    lex->input_reader->advance(lex->input_reader);
  }
  lex->input_reader->advance(lex->input_reader);
  dest->end = lex->input_reader->pos;
}

#define INSIDE_STRING_LITERAL(lex, i)          \
  lex->input_reader->text[i] != '"' ||         \
  (lex->input_reader->text[i] == '"'           \
    && lex->input_reader->text[i - 1] == '\\'  \
    && lex->input_reader->text[i - 2] != '\\')

static void lex_string_literal(lexer_t *lex, token_t *dest) {
  if (lex->input_reader->current_char == '"') {
    dest->type = string_literal;
    dest->start = lex->input_reader->pos;
    lex->input_reader->advance(lex->input_reader);
  } else {
    fprintf(
      stderr,
      "Error while lexing: String literals must begin with '\"'\n"
    );
    dest->type = syntax_error;
    dest->start = lex->input_reader->pos;
    dest->end = dest->start + 1;
    return;
  }

  while (INSIDE_STRING_LITERAL(lex, lex->input_reader->pos)) {
    lex->input_reader->advance(lex->input_reader);
  }
  // skip the closing quotation mark when done looping
  lex->input_reader->advance(lex->input_reader);

  dest->end = lex->input_reader->pos;
}

static void skip_whitespace(lexer_t *lex) {
  while (lex->input_reader->current_char &&
         isspace(lex->input_reader->current_char)) {
    lex->input_reader->advance(lex->input_reader);  
  }
}

static void skip_comment(lexer_t *lex) {
  if (lex->input_reader->current_char == '~') {
    while (lex->input_reader->current_char != '\r' &&
           lex->input_reader->current_char != '\n') {
      lex->input_reader->advance(lex->input_reader);
    }
  }

  while (lex->input_reader->current_char == '\r' ||
         lex->input_reader->current_char == '\n') {
    lex->input_reader->advance(lex->input_reader);
  }
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
       and can contain any letters, '_', or '-' */
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

    if (this->input_reader->current_char == '+') {
      result.start = this->input_reader->pos;
      result.end = result.start + 1;
      result.type = plus_sign;
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

int print_token(token_t token) {
  char *output = malloc(2 + (token.end - token.start));
  sprint_token(output, token);
  int result = printf("%s", output);
  free(output);
  return result;
}

int sprint_token(char *dest, token_t token) {
  int result = snprintf(
    dest,
    1 + token.end - token.start,
    "%s",
    token.src_text + token.start
  );
  return result;
}
