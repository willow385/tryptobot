#include <stdlib.h>
#include "dnd_lexer.h"
#include "dnd_charsheet.h"
#include "../dice.h"

void free_charsheet(charsheet_t *csp) {
  for (int i = 0; i < csp->section_count; i++) {
    for (int j = 0; j < csp->sections[i].field_count; j++) {
      if (csp->sections[i].fields[j].type == itemlist_val) {
        for (
          int k = 0;
          k < csp->sections[i].fields[j].itemlist_val.item_count;
          k++
        ) {
          free(csp->sections[i].fields[j].itemlist_val.items[k].val);
        }
        free(csp->sections[i].fields[j].itemlist_val.items);
      } else if (csp->sections[i].fields[j].type == string_val) {
        free(csp->sections[i].fields[j].string_val);
      }
      free(csp->sections[i].fields[j].identifier);
    }
    free(csp->sections[i].fields);
    free(csp->sections[i].identifier);
  }
  free(csp->sections);
  free(csp);
}
