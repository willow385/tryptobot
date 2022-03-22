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

/* This function is assigned in construct_parser() to
   the parser_t.consume() method. */
static enum parser_err parser_consume(
  parser_t *this,
  enum token_type type
) {
  if (this == NULL || this->token_vec.tokens == NULL)
    return null_ptr_error;
  if (this->token_vec.tokens[this->tok_i].type == type) {
    this->tok_i++;
    return ok;
  }
  return parser_syntax_error;
}

static inline void print_err_message(
  enum parser_err err,
  const char *expected_object
) {
  switch (err) {
    case syntax_error:
      fprintf(stderr, "Syntax error: %s expected\n", expected_object);
    break;
    case null_ptr_error:
      fprintf(stderr, "Fatal error: unexpected null pointer\n");
    break;
    case ok:
      fprintf(stderr, "Error: `print_err_message()` was called on `ok`\n");
    break;
    default:
      fprintf(stderr, "Error: unknown error code `%d`\n", err);
    break;
  }
}

/* This function is assigned in construct_parser() to
   the parser_t.parse() method. */
static charsheet_t *parser_parse(parser_t *this) {
  charsheet_t *result = malloc(sizeof(charsheet_t));
  *result = {
    .filename = this->src_filename,
    .sections = NULL,
    .section_count = 0
  };
  while (this->token_vec.tokens[this->tok_i].type != eof) {
    result->section_count++;
    result->sections = realloc(
      result->sections,
      result->section_count * sizeof(section_t)
    );
    result->sections[result->section_count - 1] = parse_section(this);
    if (result->sections[result->section_count - 1].identifier == NULL) {
      free(result->sections);
      result->sections = NULL;
    }
  }
  enum parser_err err = this->consume(this, eof);
  if (err) {
    print_err_message(err, "end of file");
  }
}

/* The field `.identifier` of this function's return value
   will be NULL in case of error. */
static section_t parse_section(parser_t *this) {
  enum parser_err err;
  section_t result = {
    .identifier = NULL,
    .fields = NULL,
    .field_count = 0
  };

  err = this->consume(this, section);
  if (err) {
    print_err_message(err, "@section");
    return result;
  }

  if (this->token_vec.tokens[this->tok_i].type == identifier) {
    result.identifier = strndup(
      this->token_vec.tokens[this->tok_i].src_text
      + this->token_vec.tokens[this->tok_i].start,
      this->token_vec.tokens[this->tok_i].end
      - this->token_vec.tokens[this->tok_i].start
    );
    this->consume(this, identifier);
  } else {
    print_err_message(parser_syntax_error, "identifier");
    return result;
  }

  err = this->consume(this, colon);
  if (err) {
    print_err_message(err, "':'");
    free(result.identifier);
    result.identifier = NULL;
    return result;
  }

  while (this->token_vec.tokens[this->tok_i].type != end_section) {
    if (this->token_vec.tokens[this->tok_i].type == eof) {
      print_err_message(parser_syntax_error, "@end-section");
      free(result.fields);
      free(result.identifier);
      result.identifier = NULL;
      return result;
    }
    field_t curr_field = parse_field(this);
    if (curr_field.type == syntax_error) {
      print_err_message(parser_syntax_error, "@field");
      free(result.fields);
      free(result.identifier);
      result.identifier = NULL;
      return result;
    }
    result.field_count++;
    result.fields = realloc(
      result.fields,
      result.field_count * sizeof(field_t)
    );
    result.fields[result.field_count - 1] = curr_field;
  }

  this->consume(this, end_section);
  return result;
}

static field_t parse_field(parser_t *this) {
  // TODO implement
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
