#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "dnd_input_reader.h"
#include "dnd_lexer.h"
#include "dnd_charsheet.h"
#include "dnd_parser.h"
#include "../dice.h"

#ifdef DEBUG_LVL
  #if DEBUG_LVL == 0
    #define DEBUG1(x) ;
    #define DEBUG2(x) ;
  #elif DEBUG_LVL == 1
    #define DEBUG1(x) x;
    #define DEBUG2(x) ;
  #elif DEBUG_LVL == 2
    #define DEBUG1(x) x;
    #define DEBUG2(x) x;
  #endif
#else
  #warning DEBUG_LVL undefined; defaulting to 0
  #define DEBUG1(x) ;
  #define DEBUG2(x) ;
#endif

static void add_token_to_vec(token_vec_t *dest, token_t token) {
  DEBUG2(
    fprintf(stderr, "~~ Inside add_token_to_vec(%p, token).\n", dest);
    fprintf(stderr, "~~   token:");
    fprint_token(stderr, token);
    fprintf(stderr, "; type: %d", token.type);
    fprintf(stderr, "\n");
  );
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
  DEBUG2(fprintf(stderr, "~~ Consuming token with type %d.\n", type));
  if (this == NULL || this->token_vec.tokens == NULL)
    return null_ptr_error;
  if (this->token_vec.tokens[this->tok_i].type == type) {
    this->tok_i++;
    return ok;
  }
  return parser_syntax_error;
}

// Prints output to last_parser_err.txt and stderr
static inline void err_message(
  enum parser_err err,
  const char *expected_object
) {
  FILE *f = fopen("/home/runner/tryptobot/dndml/errlog.txt", "w+");
  switch (err) {
    case parser_syntax_error:
      fprintf(
        stderr,
        "Syntax error: %s expected\n",
        expected_object
      );
      fprintf(
        f,
        "Syntax error: %s expected\n",
        expected_object
      );
    break;
    case null_ptr_error:
      fprintf(stderr, "Fatal error: unexpected null pointer\n");
      fprintf(f, "Fatal error: unexpected null pointer");
    break;
    case ok:
      fprintf(stderr, "Error: `err_message()` was called on `ok`\n");
      fprintf(f, "Error: `err_message()` was called on `ok`");
    break;
    default:
      fprintf(stderr, "Error: unknown error code `%d`\n", err);
      fprintf(f, "Error: unknown error code `%d`", err);
    break;
  }
  fclose(f);
}

// mandatory forward declarations
static section_t parse_section(parser_t *, enum parser_err *);
static field_t parse_field(parser_t *, enum parser_err *);
static stat_t parse_stat_val(parser_t *, enum parser_err *);
static char *parse_string_val(parser_t *, enum parser_err *);
static int parse_int_val(parser_t *, enum parser_err *);
static diceroll_t parse_dice_val(parser_t *, enum parser_err *);
static deathsave_t parse_deathsave_val(parser_t *, enum parser_err *);
static item_t parse_item_val(parser_t *, enum parser_err *);
static itemlist_t parse_itemlist_val(parser_t *, enum parser_err *);

/* This function is assigned in construct_parser() to
   the parser_t.parse() method. */
static charsheet_t *parser_parse(parser_t *this) {
  DEBUG1(fprintf(stderr, "~~ Inside parser_parse(%p).\n",  this));
  if (this->token_vec.tokens == NULL ||
      this->token_vec.token_count == 0)
    return NULL;

  enum parser_err err;
  charsheet_t *result = malloc(sizeof(charsheet_t));
  *result = (charsheet_t){
    .filename = this->src_filename,
    .sections = NULL,
    .section_count = 0
  };
  DEBUG2(
    fprintf(stderr, "~~ *result: (charsheet_t){\n");
    fprintf(stderr, "~~   .filename: %s,\n", result->filename);
    fprintf(stderr, "~~   .sections: %p,\n", result->sections);
    fprintf(stderr, "~~   .section_count: %lu\n", result->section_count);
    fprintf(stderr, "~~ }\n");
  );
  while (this->token_vec.tokens[this->tok_i].type != eof) {
    DEBUG2(
      fprintf(stderr, "~~ Inside while-loop for getting sections.\n");
      fprintf(
        stderr,
        "~~   this->token_vec.tokens[this->tok_i].type: %d\n",
        this->token_vec.tokens[this->tok_i].type
      );
    );
    result->section_count++;
    result->sections = realloc(
      result->sections,
      result->section_count * sizeof(section_t)
    );
    result->sections[result->section_count - 1] = parse_section(this, &err);
    DEBUG2(fprintf(stderr, "~~ Exited parse_section(%p).\n", this));
    if (result->sections[result->section_count - 1].identifier == NULL) {
      DEBUG1(
        fprintf(
          stderr,
          "~~ parse_section(%p) returned a section_t with a NULL identifier.\n",
          this
        );
      );
      // free_charsheet(result); // better to leak memory than to do UB
      result = NULL;
      return result;
    }
  }

  if (err) {
    free_charsheet(result);
    err_message(
      err,
      "valid syntax in file"
    );
    result = NULL;
    return result;
  }
  err = this->consume(this, eof);
  if (err) {
    free_charsheet(result);
    err_message(
      err,
      "end of file"
    );
    result = NULL;
    return result;
  }

  DEBUG2(
    if (result != NULL) {
      fprintf(stderr, "~~ *result: (charsheet_t){\n");
      fprintf(stderr, "~~   .filename: %s,\n", result->filename);
      fprintf(stderr, "~~   .sections: %p,\n", result->sections);
      fprintf(stderr, "~~   .section_count: %lu\n", result->section_count);
      fprintf(stderr, "~~ }\n");
    } else {
      fprintf(stderr, "~~ *result: *(charsheet_t *)NULL\n");
    }
  );
  return result;
}

// must have `parser_t *this` in scope
#define CONSUME_ONE_CHAR(err_ptr, type, ptr_to_free_if_err, err_str) \
  *err_ptr = this->consume(this, type);                                  \
  if (*err_ptr) {                                                        \
    err_message(*err_ptr, err_str);        \
    free(ptr_to_free_if_err);                                    \
  }

/* The field `.identifier` of this function's return value
   will be NULL in case of error. */
static section_t parse_section(parser_t *this, enum parser_err *err) {
  DEBUG1(fprintf(stderr, "~~ Inside parse_section(%p).\n", this));
  section_t result = {
    .identifier = NULL,
    .fields = NULL,
    .field_count = 0
  };

  DEBUG2(
    fprintf(stderr, "~~ result: (section_t){\n");
    fprintf(stderr, "~~   .identifier: %p,\n", result.identifier);
    fprintf(stderr, "~~   .fields: %p,\n", result.fields);
    fprintf(stderr, "~~   .field_count: %lu\n", result.field_count);
    fprintf(stderr, "~~ }\n");
  );

  *err = this->consume(this, section);
  if (*err) {
    err_message(*err, "@section");
    return result;
  }

  if (this->token_vec.tokens[this->tok_i].type == identifier) {
    result.identifier = strndup(
      this->token_vec.tokens[this->tok_i].src_text
      + this->token_vec.tokens[this->tok_i].start,
      this->token_vec.tokens[this->tok_i].end
      - this->token_vec.tokens[this->tok_i].start
    );
    DEBUG2(fprintf(stderr, "~~ Assigning to result.identifier: %s\n", result.identifier));
    this->consume(this, identifier);
  } else {
    *err = parser_syntax_error;
    err_message(*err, "section identifier");
    return result;
  }

  CONSUME_ONE_CHAR(err, colon, result.identifier, "':'");

  while (this->token_vec.tokens[this->tok_i].type != end_section) {
    // if (*err) break;
    if (this->token_vec.tokens[this->tok_i].type == eof) {
      *err = parser_syntax_error;
      err_message(*err, "@end-section");
      free(result.fields);
      free(result.identifier);
      result.identifier = NULL;
      return result;
    }
    field_t curr_field = parse_field(this, err);
    DEBUG1(fprintf(stderr, "~~ Exited parse_field(this).\n"));
    if (curr_field.type == syntax_error) {
      DEBUG1(
        fprintf(stderr, "~~ curr_field.type == syntax_error is true.\n");
        fprintf(stderr, "~~ this->token_vec.tokens[this->tok_i]: ");
        fprint_token(stderr, this->token_vec.tokens[this->tok_i]);
        fprintf(stderr, "~~ curr_field.string_val: %p\n", curr_field.string_val);
      );
      *err = parser_syntax_error;
      err_message(*err, "@field");
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
    if (this->token_vec.tokens[this->tok_i].type == end_section) {
      break;
    } else if (this->token_vec.tokens[this->tok_i].type == field) {
      continue;
    } else {
      DEBUG1(fprintf(
        stderr, "~~ Error: expected type %d or %d, got %d\n",
        semicolon, end_section, this->token_vec.tokens[this->tok_i].type
      ));
    }
  }

  *err = this->consume(this, end_section);
  DEBUG2(
    fprintf(stderr, "~~ result: (section_t){\n");
    if (result.identifier)
      fprintf(stderr, "~~   .identifier: %p => %s,\n",
        result.identifier, result.identifier);
    else
      fprintf(stderr, "~~   .identifier: %p,\n", result.identifier);
    fprintf(stderr, "~~   .fields: %p,\n", result.fields);
    fprintf(stderr, "~~   .field_count: %lu\n", result.field_count);
    fprintf(stderr, "~~ }\n");
  );
  return result;
}

static field_t parse_field(parser_t *this, enum parser_err *err) {
  DEBUG1(fprintf(stderr, "~~ Inside parse_field(%p).\n", this));
  field_t result = {
    .int_val = INT_MIN,
    .identifier = NULL,
    .type = syntax_error
  };

  DEBUG2(
    fprintf(stderr, "~~ result: (field_t){\n");
    fprintf(stderr, "~~   .identifier: %p,\n", result.identifier);
    fprintf(stderr, "~~   .int_val: %d,\n", result.int_val);
    fprintf(stderr, "~~   .type: %d\n", result.type);
    fprintf(stderr, "~~ }\n");
  );

  *err = this->consume(this, field);
  if (*err) {
    err_message(*err, "@field");
    return result;
  }

  if (this->token_vec.tokens[this->tok_i].type == identifier) {
    result.identifier = strndup(
      this->token_vec.tokens[this->tok_i].src_text
      + this->token_vec.tokens[this->tok_i].start,
      this->token_vec.tokens[this->tok_i].end
      - this->token_vec.tokens[this->tok_i].start
    );
    DEBUG2(fprintf(stderr, "~~ identifier address: %p\n", result.identifier));
    DEBUG2(fprintf(stderr, "~~ identifier: %s\n", result.identifier));
    *err = this->consume(this, identifier);
  } else {
    *err = parser_syntax_error;
    err_message(*err, "field identifier");
    return result;
  }

  CONSUME_ONE_CHAR(err, colon, result.identifier, "':'");

  result.type = this->token_vec.tokens[this->tok_i].type;
  DEBUG2(fprintf(stderr, "~~ from parse_field(%p): ", this));
  DEBUG2(fprint_token(stderr, this->token_vec.tokens[this->tok_i]));
  DEBUG2(fprintf(stderr, "; type: %d\n", result.type));
  switch (result.type) {
    case stat_val:
      result.stat_val = parse_stat_val(this, err);
    break;
    case string_val:
      DEBUG2(fprintf(stderr, "~~ trying to parse string\n"));
      result.string_val = parse_string_val(this, err);
      DEBUG2(fprintf(stderr, "~~ Exited parse_string_val(%p).\n", this));
    break;
    case int_val:
      result.int_val = parse_int_val(this, err);
    break;
    case dice_val:
      result.dice_val = parse_dice_val(this, err);
    break;
    case deathsave_val:
      result.deathsave_val = parse_deathsave_val(this, err);
    break;
    case itemlist_val:
      result.itemlist_val = parse_itemlist_val(this, err);
    break;
    case item_val:
      result.item_val = parse_item_val(this, err);
    break;
    default:
      *err = parser_syntax_error;
      err_message(*err, "field value");
    break;
  }

  DEBUG2(
    fprintf(stderr, "~~ result: (field_t){\n");
    if (result.identifier)
      fprintf(stderr, "~~   .identifier: %p => %s,\n",
        result.identifier, result.identifier);
    else
      fprintf(stderr, "~~   .identifier: %p,\n", result.identifier);
    fprintf(stderr, "~~   .int_val: %d,\n", result.int_val);
    fprintf(stderr, "~~   .type: %d\n", result.type);
    fprintf(stderr, "~~ }\n");
  );

  if (*err) {
    free(result.identifier);
    result.type = syntax_error;
    return result;
  }

  if (this->token_vec.tokens[this->tok_i].type == semicolon) {
    *err = this->consume(this, semicolon);
  } else if (this->token_vec.tokens[this->tok_i].type != end_section) {
    *err = parser_syntax_error;
    err_message(*err, "';' or @end-section");
    fprintf(stderr, "~~ Got ");
    fprint_token(stderr, this->token_vec.tokens[this->tok_i]);
    fprintf(stderr, " instead\n");
    DEBUG2(
      fprintf(stderr, "~~ result: (field_t){\n");
      fprintf(
        stderr,
        "~~   .identifier: %p => \"%s\",\n",
        result.identifier,
        result.identifier
      );
      if (result.type == string_val && result.string_val != NULL) {
        fprintf(stderr, "~~   .string_val: %s,\n", result.string_val);
      }
      fprintf(stderr, "~~   .int_val: %d,\n", result.int_val);
      fprintf(stderr, "~~   .type: %d\n", result.type);
      fprintf(stderr, "~~ }\n");
    );
    free(result.identifier);
    result.identifier = NULL;
    if (result.type == string_val) {
      free(result.string_val);
    } else if (result.type == itemlist_val) {
      free(result.itemlist_val.items);
    }
    return result;
  }

  DEBUG2(
    fprintf(stderr, "~~ result: (field_t){\n");
    fprintf(
      stderr,
      "~~   .identifier: %p => %s,\n",
      result.identifier,
      result.identifier
    );
    if (result.type == string_val && result.string_val != NULL) {
      fprintf(stderr, "~~   .string_val: %s,\n", result.string_val);
    }
    fprintf(stderr, "~~   .int_val: %d,\n", result.int_val);
    fprintf(stderr, "~~   .type: %d\n", result.type);
    fprintf(stderr, "~~ }\n");
  );
  return result;
}

// TODO parameterize err
#define CONSUME_INT_OR_NULL(out_int_ptr)                             \
  if (this->token_vec.tokens[this->tok_i].type == int_literal) {     \
    sscanf(                                                          \
      this->token_vec.tokens[this->tok_i].src_text +                 \
      this->token_vec.tokens[this->tok_i].start,                     \
      "%d",                                                          \
      out_int_ptr                                                    \
    );                                                               \
    this->consume(this, int_literal);                                \
  } else if (this->token_vec.tokens[this->tok_i].type == null_val) { \
    this->consume(this, null_val);                                   \
  } else {                                                           \
    *err = parser_syntax_error;\
    err_message(*err, "integer or NULL");             \
    return result;                                                   \
  }

#define CONSUME_RESERVED_IDENTIFIER(err_ptr, _str, identifier_str) \
  _str = strndup(this->token_vec.tokens[this->tok_i].src_text  \
    + this->token_vec.tokens[this->tok_i].start,\
    this->token_vec.tokens[this->tok_i].end       \
    - this->token_vec.tokens[this->tok_i].start\
  );\
  if (strcmp(_str, identifier_str)) {             \
    *err_ptr = parser_syntax_error;                    \
    err_message(*err_ptr, identifier_str);           \
    fprintf(stderr, "Got \"%s\" instead\n", _str);\
    free(_str);                                \
  } else {                                        \
    free(_str);\
    *err_ptr = this->consume(this, identifier);        \
  }

static stat_t parse_stat_val(parser_t *this, enum parser_err *err) {
  stat_t result = { .ability = INT_MIN, .mod = INT_MIN };
  char *buf;

  *err = this->consume(this, stat_val);

  CONSUME_ONE_CHAR(err, open_sqr_bracket, NULL, "'['");

  CONSUME_RESERVED_IDENTIFIER(err, buf, "ability");
  if (*err) {
    err_message(*err, "\"ability\"");
    return result;
  }

  CONSUME_ONE_CHAR(err, colon, NULL, "':'");

  CONSUME_INT_OR_NULL(&result.ability);

  CONSUME_ONE_CHAR(err, semicolon, NULL, "';'");

  CONSUME_RESERVED_IDENTIFIER(err, buf, "mod");
  if (*err) {
    err_message(*err, "\"mod\"");
    return result;
  }

  CONSUME_ONE_CHAR(err, colon, NULL, "':'");

  CONSUME_INT_OR_NULL(&result.mod);

  CONSUME_ONE_CHAR(err, close_sqr_bracket, NULL, "']'");

  return result;
}

static char *parse_string_val(parser_t *this, enum parser_err *err) {
  DEBUG1(fprintf(stderr, "~~ Inside parse_string_val(%p).\n", this));
  char *result = NULL;

  this->consume(this, string_val);

  CONSUME_ONE_CHAR(err, open_sqr_bracket, NULL, "'['");

  if (this->token_vec.tokens[this->tok_i].type == string_literal) {
    // The token will contain quotation marks at the beginning and end,
    // so we allocate _fewer_ chars than what the token points to.
    size_t bufsize = this->token_vec.tokens[this->tok_i].end -
                     this->token_vec.tokens[this->tok_i].start - 1;
    result = malloc(bufsize);
    if (result == NULL) {
      *err = null_ptr_error;
      err_message(*err, "non-null pointer from malloc()");
      return result;
    }
    for (int i = 0; i < bufsize - 1; i++) {
      result[i] = (
        this->token_vec.tokens[this->tok_i].src_text +
        this->token_vec.tokens[this->tok_i].start + 1
      )[i];
    }
    result[bufsize - 1] = '\0';
    *err = this->consume(this, string_literal);
  } else if (this->token_vec.tokens[this->tok_i].type == null_val) {
    *err = this->consume(this, null_val);
  } else DEBUG2({
    fprintf(
      stderr,
      "~~ Unexpected token of type %d encountered.\n",
      this->token_vec.tokens[this->tok_i].type
    );
    fprintf(stderr, "~~ this->token_vec.tokens[this->tok_i]: ");
    fprint_token(stderr, this->token_vec.tokens[this->tok_i]);
    fprintf(stderr, "\n");
  });

  DEBUG2(
    if (result == NULL) {
      fprintf(stderr, "~~   result: %p\n", result);
    } else {
      fprintf(stderr, "~~   result: %p => %s\n", result, result);
    }
  );

  DEBUG2(
    fprintf(stderr, "~~ Current token: ");
    fprint_token(stderr, this->token_vec.tokens[this->tok_i]);
    fprintf(stderr, "; type: %d\n",
      this->token_vec.tokens[this->tok_i].type);
    fprintf(stderr, "~~ Consuming next token...\n");
  );
  CONSUME_ONE_CHAR(err, close_sqr_bracket, NULL, "']'");
  DEBUG2(
    if (*err) {
      fprintf(
        stderr,
        "~~ Syntax error: expected type %d, got type %d\n",
        close_sqr_bracket,
        this->token_vec.tokens[this->tok_i].type
      );
      fprintf(stderr, "~~ Token: ");
      fprint_token(stderr, this->token_vec.tokens[this->tok_i]);
      printf("\n");
    }
    fprintf(stderr, "~~ Current token: ");
    fprint_token(stderr, this->token_vec.tokens[this->tok_i]);
    fprintf(stderr, "; type: %d\n",
      this->token_vec.tokens[this->tok_i].type);
  );

  return result;
}

static int parse_int_val(parser_t *this, enum parser_err *err) {
  int result = INT_MIN;

  this->consume(this, int_val);

  CONSUME_ONE_CHAR(err, open_sqr_bracket, NULL, "'['");

  CONSUME_INT_OR_NULL(&result);

  CONSUME_ONE_CHAR(err, close_sqr_bracket, NULL, "']'");

  return result;
}

static diceroll_t parse_dice_val(parser_t *this, enum parser_err *err) {
  diceroll_t result = {
    .dice_ct = INT_MIN,
    .faces = INT_MIN,
    .modifier = INT_MIN,
    .value = INT_MIN
  };
  char *buf;

  *err = this->consume(this, dice_val);

  CONSUME_ONE_CHAR(err, open_sqr_bracket, NULL, "'['");

  CONSUME_INT_OR_NULL(&result.dice_ct);

  CONSUME_RESERVED_IDENTIFIER(err, buf, "d");
  if (*err) {
    err_message(*err, "'d'");
    return result;
  }

  CONSUME_INT_OR_NULL(&result.faces);

  CONSUME_ONE_CHAR(err, plus_sign, NULL, "'+'");

  CONSUME_INT_OR_NULL(&result.modifier);

  CONSUME_ONE_CHAR(err, close_sqr_bracket, NULL, "']'");

  result.value = 0; // unused by dndml
  return result;
}

static deathsave_t parse_deathsave_val(parser_t *this, enum parser_err *err) {
  deathsave_t result = { .succ = INT_MIN, .fail = INT_MIN };
  char *buf;

  *err = this->consume(this, deathsave_val);

  CONSUME_ONE_CHAR(err, open_sqr_bracket, NULL, "'['");

  CONSUME_RESERVED_IDENTIFIER(err, buf, "succ");
  if (*err) {
    err_message(*err, "\"succ\"");
    return result;
  }

  CONSUME_ONE_CHAR(err, colon, NULL, "':'");

  CONSUME_INT_OR_NULL(&result.succ);

  CONSUME_ONE_CHAR(err, semicolon, NULL, "';'");

  CONSUME_RESERVED_IDENTIFIER(err, buf, "fail");
  if (*err) {
    err_message(*err, "\"fail\"");
    return result;
  }

  CONSUME_ONE_CHAR(err, colon, NULL, "':'");

  CONSUME_INT_OR_NULL(&result.fail);

  CONSUME_ONE_CHAR(err, close_sqr_bracket, NULL, "']'");

  return result;
}

static item_t parse_item_val(parser_t *this, enum parser_err *err) {
  DEBUG2(fprintf(stderr, "~~ Inside parse_item_val(%p).\n", this));
  item_t result = {
    .val = NULL,
    .qty = INT_MIN,
    .weight = INT_MIN
  };
  char *buf;

  *err = this->consume(this, item_val);

  CONSUME_ONE_CHAR(err, open_sqr_bracket, NULL, "'['");

  CONSUME_RESERVED_IDENTIFIER(err, buf, "val");
  if (*err) {
    err_message(*err, "\"val\"");
    return result;
  }

  CONSUME_ONE_CHAR(err, colon, NULL, "':'");

  if (this->token_vec.tokens[this->tok_i].type == string_literal) {
    size_t bufsize = this->token_vec.tokens[this->tok_i].end -
                     this->token_vec.tokens[this->tok_i].start - 1;
    result.val = malloc(bufsize);
    for (int i = 0; i < bufsize - 1; i++) {
      result.val[i] = (
        this->token_vec.tokens[this->tok_i].src_text +
        this->token_vec.tokens[this->tok_i].start + 1
      )[i];
    }
    result.val[bufsize - 1] = '\0';
    this->consume(this, string_literal);
  } else if (this->token_vec.tokens[this->tok_i].type == null_val) {
    this->consume(this, null_val);
  } else {
    err_message(parser_syntax_error, "string literal or NULL");
    /* TODO implement more robust error handling */
  }

  CONSUME_ONE_CHAR(err, semicolon, result.val, "';'");

  CONSUME_RESERVED_IDENTIFIER(err, buf, "qty");
  if (*err) {
    err_message(*err, "\"qty\"");
    free(result.val);
    result.val = NULL;
    return result;
  }

  CONSUME_ONE_CHAR(err, colon, result.val, "':'");

  CONSUME_INT_OR_NULL(&result.qty);

  CONSUME_ONE_CHAR(err, semicolon, result.val, "';'");

  CONSUME_RESERVED_IDENTIFIER(err, buf, "weight");
  if (*err) {
    err_message(*err, "\"weight\"");
    free(result.val);
    result.val = NULL;
    DEBUG2(fprintf(stderr,
      "~~ Returning from parse_item_val(%p).\n", this));
    return result;
  }

  CONSUME_ONE_CHAR(err, colon, result.val, "':'");

  CONSUME_INT_OR_NULL(&result.weight);

  CONSUME_ONE_CHAR(err, close_sqr_bracket, result.val, "']'");

  CONSUME_ONE_CHAR(err, semicolon, result.val, "';'");

  DEBUG2(fprintf(stderr,
    "~~ Returning from parse_item_val(%p).\n", this));

  return result;
}

static itemlist_t parse_itemlist_val(parser_t *this, enum parser_err *err) {
  DEBUG2(fprintf(stderr, "~~ Inside parse_itemlist_val(%p).\n", this));
  itemlist_t result = {
    .items = NULL,
    .item_count = 0
  };

  *err = this->consume(this, itemlist_val);

  CONSUME_ONE_CHAR(err, open_sqr_bracket, NULL, "'['");

  while (this->token_vec.tokens[this->tok_i].type != close_sqr_bracket) {
    if (this->token_vec.tokens[this->tok_i].type == eof) {
      *err = parser_syntax_error;
      err_message(*err, "']'");
      free(result.items);
      result.item_count = 0;
      DEBUG2(fprintf(stderr,
          "~~ Returning from parse_itemlist_val(%p).\n", this));
      return result;
    }
    item_t item = parse_item_val(this, err);
    result.item_count++;
    result.items = realloc(
      result.items, result.item_count * sizeof(item_t)
    );
    result.items[result.item_count - 1] = item;
    if (this->token_vec.tokens[this->tok_i].type == close_sqr_bracket) {
      break;
    }
  }

  if (*err) {
    free(result.items);
    result.item_count = 0;
    return result;
  }

  *err = this->consume(this, close_sqr_bracket);
  DEBUG2(fprintf(stderr,
            "~~ Returning from parse_itemlist_val(%p).\n", this));
  return result;
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
      FILE *log = fopen(
        "/home/runner/tryptobot/dndml/errlog.txt", "w+"
      );
      fprintf(
        stderr,
        "Syntax error in token stream generated while parsing.\n"
      );
      fprintf(
        log,
        "Syntax error in token stream generated while parsing.\n"
      );
      fclose(log);
      free(dest->token_vec.tokens);
      dest->token_vec.tokens = NULL;
      dest->token_vec.token_count = 0;
      break;
    }
    add_token_to_vec(&dest->token_vec, current_token);
  } while (current_token.type != eof);
  dest->tok_i = 0;
}
