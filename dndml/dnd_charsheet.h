#include <stdio.h>
#include "dnd_lexer.h"
#include "../dice.h"

#ifndef DND_CHARSHEET_H
#define DND_CHARSHEET_H

struct stat { int ability, mod; }; // fields set to INT_MIN if NULL
struct deathsave { int succ, fail; }; // ditto above

struct item {
  char *val; // heap-allocated
  int qty, weight; // set to INT_MIN if NULL
};

struct itemlist {
  struct item *items; // heap-allocated
  size_t item_count;
};

typedef struct field {
  union {
    struct stat stat_val; // %stat
    char *string_val; // %string (heap-allocated)
    int int_val; // %int (set to INT_MIN if NULL)
    diceroll_t dice_val; // %dice (has `.value` set to INT_MIN if NULL)
    struct deathsave deathsave_val; // %deathsaves
    struct itemlist itemlist_val; // %itemlist
    struct item item_val; // %item
  };
  char *identifier; // heap-allocated
  enum token_type type;
} field_t;

typedef struct section {
  char *identifier; // heap-allocated
  field_t *fields; // heap-allocated
  size_t field_count;
} section_t;

typedef struct charsheet {
  char *filename;
  section_t *sections; // heap-allocated
  size_t section_count;
} charsheet_t;

// Frees csp and its members. *csp must be on the heap
void free_charsheet(charsheet_t *csp);

char *charsheet_to_str(charsheet_t *csp);

#endif // DND_CHARSHEET_H
