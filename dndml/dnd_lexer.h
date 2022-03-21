#include "dnd_input_reader.h"

#ifndef DND_LEXER_H
#define DND_LEXER_H

enum token_type {
  eof = -1,

  /**
   * Reserved words have values explicitly assigned 
   * in  order to make it explicit and obvious that 
   * they can  be used as indices into  `const char 
   * *reserved_words[]`,  defined further  down  in 
   * this file.
   */
  section = 0,       // @section
  end_section = 1,   // @end-section
  field = 2,         // @field
  stat_val = 3,      // %stat
  string_val = 4,    // %string
  int_val = 5,       // %int
  dice_val = 6,      // %dice
  deathsave_val = 7, // %deathsaves
  itemlist_val = 8,  // %itemlist
  item_val = 9,      // %item
  null_val = 10,     // NULL
  identifier,        // what might come right after @section or @field

  plus_sign,
  colon,
  semicolon,
  open_sqr_bracket,
  close_sqr_bracket,
  int_literal,
  string_literal,

  syntax_error
};

extern const char *reserved_words[];

extern size_t reserved_word_count;

typedef struct token {
  const char *src_text; // non-owning
  size_t start; // index of the first char of the token
  size_t end; // index of the char right after the last char of the token
  enum token_type type;
} token_t;

typedef struct lexer {
  input_reader_t *input_reader;
  token_t (*get_next_token)(struct lexer *);
} lexer_t;

void construct_lexer(lexer_t *dest, input_reader_t *ir);

int print_token(token_t token);
int sprint_token(char *dest, token_t token);

#endif // DND_LEXER_H
