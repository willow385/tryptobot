#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
#include "dnd_lexer.h"
#include "dnd_charsheet.h"
#include "../dice.h"

static char *dstrcatf(char *dest, const char *format, ...) {
  va_list args;
  va_start(args, format);
  size_t bufsize;
  if (dest != NULL)
    bufsize = 1 + vsnprintf(NULL, 0, format, args) + strlen(dest);
  else
    bufsize = 1 + vsnprintf(NULL, 0, format, args);
  dest = realloc(dest, bufsize);
  vsprintf(dest + strlen(dest), format, args);
  va_end(args);
  return dest;
}

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

char *charsheet_to_str(charsheet_t *csp) {
  int time_of_call = time(); // TODO: this is vulnerable to the 2038 problem
  char *result = NULL;
  result = dstrcatf(
    result,
    "~~ Character sheet autogenerated by tryptobot.\n"
    "~~ Timestamp: %d\n\n",
    time_of_call
  );

  for (int i = 0; i < csp->section_count; i++) {
    result = dstrcatf(
      result,
      "@section %s:\n",
      csp->sections[i].identifier
    );
    for (int j = 0; j < csp->sections[i].field_count; j++) {
      result = dstrcatf(result, "  @field: ");
      switch (csp->sections[i].fields[j].type) {
        case stat_val:
          result = dstrcatf(result, "%%stat[ability:",);
          if (csp->sections[i].fields[j].stat_val.ability == INT_MIN) {
            result = dstrcatf(result, "NULL;mod:");
          } else {
            result = dstrcatf(
              result, "%d;mod:",
              csp->sections[i].fields[j].stat_val.ability
            );
          }
          if (csp->sections[i].fields[j].stat_val.mod == INT_MIN) {
            result = dstrcatf(result, "NULL];\n");
          } else {
            result = dstrcatf(
              result, "%d];\n",
              csp->sections[i].fields[j].stat_val.mod
            );
          }
        break;
        case string_val:
          if (csp->sections[i].fields[j].string_val == NULL) {
            result = dstrcatf(result, "%%string[NULL];\n");
          } else {
            result = dstrcatf(
              result,
              "%%string[\"%s\"];\n",
              csp->sections[i].fields[j].string_val
            );
          }
        break;
        case int_val:
          if (csp->sections[i].fields[j].int_val == INT_MIN) {
            result = dstrcatf(result, "%%int[NULL];\n");
          } else {
            result = dstrcatf(
              result,
              "%%int[%d];\n",
              csp->sections[i].fields[j].int_val
            );
          }
        break;
        case dice_val:
          result = dstrcatf(result, "%%dice[");
          if (csp->sections[i].fields[j].dice_val.value == INT_MIN) {
            result = dstrcatf(result, "%%dice[NULL];\n");
          } else {
            result = dstrcatf(
              result,
              "%%dice[%dd%d+%d];\n",
              csp->sections[i].fields[j].dice_val.dice_ct,
              csp->sections[i].fields[j].dice_val.faces,
              csp->sections[i].fields[j].dice_val.modifier
            );
          }
        break;
        case deathsave_val:
          result = dstrcatf(result, "%%deathsaves[succ:");
          if (csp->sections[i].fields[j].deathsave_val.succ == INT_MIN) {
            result = dstrcatf(result, "NULL;fail:")
          } else {
            result = dstrcatf(
              result, "%d;fail:",
              csp->sections[i].fields[j].deathsave_val.succ
            );
          }
          if (csp->sections[i].fields[j].deathsave_val.fail == INT_MIN) {
            result = dstrcatf(result, "NULL];\n");
          } else {
            result = dstrcatf(
              result, "%d];\n",
              csp->sections[i].fields[j].deathsave_val.fail
            );
          }
        break;
        case itemlist_val:
          result = dstrcatf(result, "%%itemlist[\n");
          for (
            int k = 0;
            k < csp->sections[i].fields[j].itemlist_val.item_count;
            k++
          ) {
            result = dstrcatf(result, "    %%item[val:");
            if (csp->sections[i].fields[j].itemlist_val[k].val == NULL) {
              result = dstrcatf(result, "NULL;qty:");
            } else {
              result = dstrcatf(
                result,
                "\"%s\";qty:",
                csp->sections[i].fields[j].itemlist_val[k].val
              );
            }
            if (csp->sections[i].fields[j].itemlist_val[k].qty == INT_MIN) {
              result = dstrcatf(result, "NULL;weight:");
            } else {
              result = dstrcatf(
                result,
                "%d;weight:",
                csp->sections[i].fields[j].itemlist_val[k].qty
              );
            }
            if (csp->sections[i].fields[j].itemlist_val[k].qty == INT_MIN) {
              result = dstrcatf(result, "NULL];\n");
            } else {
              result = dstrcatf(
                result,
                "%d];\n",
                csp->sections[i].fields[j].itemlist_val[k].weight
              );
            }
          }
          result = strcatf(result, "  ];\n");
        break;
        case item_val:
          result = dstrcatf(result, "    %%item[val:");
          if (csp->sections[i].fields[j].item_val.val == NULL) {
            result = dstrcatf(result, "NULL;qty:");
          } else {
            result = dstrcatf(
              result,
              "\"%s\";qty:",
              csp->sections[i].fields[j].item_val.val
            );
          }
          if (csp->sections[i].fields[j].item_val.qty == INT_MIN) {
            result = dstrcatf(result, "NULL;weight:");
          } else {
            result = dstrcatf(
              result,
              "%d;weight:",
              csp->sections[i].fields[j].item_val.qty
            );
          }
          if (csp->sections[i].fields[j].item_val.qty == INT_MIN) {
            result = dstrcatf(result, "NULL];\n");
          } else {
            result = dstrcatf(
              result,
              "%d];\n",
              csp->sections[i].fields[j].item_val.weight
            );
          }
        break;
        default:
          free(result);
          return NULL;
        break;
      }
    }
    result = dstrcatf(result, "@end-section\n\n");
  }

  return result;
}
