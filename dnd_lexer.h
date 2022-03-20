#include "dnd_input_reader.h"

enum token_type {
  eof = -1,
  section = 0, // @section
  end_section, // @end-section
  field,       // @field
  identifier,  // what might come right after @section or @field
  stat_val,    // %stat
  string_val,  // %string
  int_val,     // %int
  dice_val,    // %dice
  deathsave_val, // %deathsaves
  itemlist_val, // %itemlist
  item_val,    // %item
  /* TODO enumerate other token types */
};

typedef struct token {
  const char *src_text; // non-owning
  size_t start; // index of the first char of the token
  size_t end; // index of the last char of the token
} token_t;

typedef struct lexer {
  input_reader_t *ir;
  token_t (*get_next_token)(struct lexer *);
} lexer_t;
