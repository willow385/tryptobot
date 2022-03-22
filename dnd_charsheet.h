#include <stdio.h>
#include "dnd_lexer.h"
#include "../dice.h"

#ifndef DND_CHARSHEET_H
#define DND_CHARSHEET_H

struct stat { int ability, mod; };
struct deathsave { int succ, fail; };

struct item {
  char *val;
  int qty, weight;
};

struct itemlist {
  struct item *items;
  size_t item_count;
};

typedef struct field {
  union {
    struct stat stat_val; // %stat
    char *string_val; // %string
    int int_val; // %int
    diceroll_t dice_val; // %dice
    struct deathsave deathsave_val; // %deathsaves
    struct itemlist itemlist_val; // %itemlist
    struct item item_val; // %item
  };
  char *identifier; // heap-allocated
  enum token_type type;
} field_t;

typedef struct section {
  char *identifier; // heap-allocated
  field_t *fields;
  size_t field_count;
} section_t;

typedef struct charsheet {
  char *filename;
  section_t *sections;
  size_t section_count;
} charsheet_t;

// Frees a heap-allocated charsheet_t and its members.
void free_charsheet(charsheet_t *csp);

#endif // DND_CHARSHEET_H
