#ifndef DND_CHARSHEET_H
#define DND_CHARSHEET_H

#include <stdio.h>
#include <math.h>
#include "dnd_lexer.h"
#include "../dice.h"

// fields set to INT_MIN if NULL
typedef struct stat { int ability, mod; } stat_t;
typedef struct deathsave { int succ, fail; } deathsave_t;

typedef struct item {
  char *val; // heap-allocated
  int qty; // set to INT_MIN if NULL
  float weight; // set to NAN if NULL
} item_t;

typedef struct itemlist {
  item_t *items; // heap-allocated
  size_t item_count;
} itemlist_t;

typedef struct field {
  union {
    stat_t stat_val; // %stat
    char *string_val; // %string (heap-allocated)
    int int_val; // %int (set to INT_MIN if NULL)
    float float_val; // %float (set to NAN if NULL)
    diceroll_t dice_val; // %dice (has `.value` set to INT_MIN if NULL)
    deathsave_t deathsave_val; // %deathsaves
    itemlist_t itemlist_val; // %itemlist
    item_t item_val; // %item
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
  char *filename; // not owned by the charsheet_t object
  section_t *sections; // heap-allocated
  size_t section_count;
} charsheet_t;

// Frees csp and its members. *csp must be on the heap
void free_charsheet(charsheet_t *csp);

char *charsheet_to_str(charsheet_t *csp);

#endif // DND_CHARSHEET_H
