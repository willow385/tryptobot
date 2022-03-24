#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#include "dnd_lexer.h"
#include "dnd_charsheet.h"
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

#define GLOBAL_BUF_SIZE 2048UL
static char global_buf[GLOBAL_BUF_SIZE];

static char *dstrcat(char *dest, const char *str_to_append) {
  DEBUG1(
    fprintf(stderr, "~~ Calling dstrcat(");
    if (dest) {
      fprintf(stderr, "\"%s\", \"%s\").\n", dest, str_to_append);
    } else {
      fprintf(stderr, "%p, \"%s\").\n", dest, str_to_append);
    }
    fprintf(stderr, "~~ ============== ~~\n");
  );
  size_t bufsize;
  if (dest != NULL)
    bufsize = 1 + snprintf(NULL, 0, "%s", str_to_append) + strlen(dest);
  else
    bufsize = 1 + snprintf(NULL, 0, "%s", str_to_append);
  DEBUG2(
    fprintf(
      stderr, "~~   Allocating %lu chars to dest.\n", bufsize
    );
    fprintf(
      stderr,
      "~~   strlen(dest): %lu\n",
      dest == NULL ? 0 : strlen(dest)
    );
  );
  _Bool should_not_strlen_dest = dest == NULL;
  dest = realloc(dest, bufsize);
  if (should_not_strlen_dest)
    sprintf(dest, "%s", str_to_append);
  else
    sprintf(dest + strlen(dest), "%s", str_to_append);

  DEBUG2(fprintf(stderr, "~~   Result: \"%s\"\n", dest));
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
  char *result = NULL;
  int timestamp = time(NULL);
  snprintf(
    global_buf, GLOBAL_BUF_SIZE,
    "~~ Character sheet autogenerated by tryptobot.\n"
    "~~ Timestamp: %d\n\n",
    timestamp // TODO: this is vulnerable to the 2038 problem
  );
  result = dstrcat(result, global_buf);

  for (int i = 0; i < csp->section_count; i++) {
    DEBUG2(
      fprintf(
        stderr,
        "~~   Attempting to cat \"%s\".\n",
        csp->sections[i].identifier
      );
    );
    snprintf(global_buf, GLOBAL_BUF_SIZE, "@section %s:\n",
      csp->sections[i].identifier);
    result = dstrcat(
      result,
      global_buf
    );
    for (int j = 0; j < csp->sections[i].field_count; j++) {
      DEBUG2(
        fprintf(
          stderr,
          "~~   Attempting to cat \"%s\".\n",
          csp->sections[i].fields[j].identifier
        );
      );
      snprintf(
        global_buf,
        GLOBAL_BUF_SIZE,
        "  @field %s: ",
        csp->sections[i].fields[j].identifier
      );
      result = dstrcat(
        result,
        global_buf
      );
      switch (csp->sections[i].fields[j].type) {
        case stat_val:
          result = dstrcat(result, "%stat[ability:");
          if (csp->sections[i].fields[j].stat_val.ability == INT_MIN) {
            result = dstrcat(result, "NULL;mod:");
          } else {
            snprintf(global_buf, GLOBAL_BUF_SIZE, "%d;mod:",
              csp->sections[i].fields[j].stat_val.ability);
            result = dstrcat(result, global_buf);
          }
          if (csp->sections[i].fields[j].stat_val.mod == INT_MIN) {
            result = dstrcat(result, "NULL];\n");
          } else {
            snprintf(global_buf, GLOBAL_BUF_SIZE, "%d];\n",
              csp->sections[i].fields[j].stat_val.mod);
            result = dstrcat(result, global_buf);
          }
        break;
        case string_val:
          if (csp->sections[i].fields[j].string_val == NULL) {
            result = dstrcat(result, "%string[NULL];\n");
          } else {
            snprintf(global_buf, GLOBAL_BUF_SIZE,
              "%%string[\"%s\"];\n",
              csp->sections[i].fields[j].string_val
            );
            result = dstrcat(result, global_buf);
          }
        break;
        case int_val:
          if (csp->sections[i].fields[j].int_val == INT_MIN) {
            result = dstrcat(result, "%int[NULL];\n");
          } else {
            snprintf(global_buf, GLOBAL_BUF_SIZE,
              "%%int[%d];\n",
              csp->sections[i].fields[j].int_val);
            result = dstrcat(
              result,
              global_buf
            );
          }
        break;
        case dice_val:
          result = dstrcat(result, "%dice[");
          if (csp->sections[i].fields[j].dice_val.value == INT_MIN) {
            result = dstrcat(result, "NULL];\n");
          } else {
            snprintf(
              global_buf, GLOBAL_BUF_SIZE,
              "%dd%d+%d];\n",
              csp->sections[i].fields[j].dice_val.dice_ct,
              csp->sections[i].fields[j].dice_val.faces,
              csp->sections[i].fields[j].dice_val.modifier
            );
            result = dstrcat(result, global_buf);
          }
        break;
        case deathsave_val:
          result = dstrcat(result, "%deathsaves[succ:");
          if (csp->sections[i].fields[j].deathsave_val.succ == INT_MIN) {
            result = dstrcat(result, "NULL;fail:");
          } else {
            snprintf(
              global_buf, GLOBAL_BUF_SIZE, "%d;fail:",
              csp->sections[i].fields[j].deathsave_val.succ
            );
            result = dstrcat(result, global_buf);
          }
          if (csp->sections[i].fields[j].deathsave_val.fail == INT_MIN) {
            result = dstrcat(result, "NULL];\n");
          } else {
            snprintf(
              global_buf, GLOBAL_BUF_SIZE, "%d];\n",
              csp->sections[i].fields[j].deathsave_val.fail
            );
            result = dstrcat(result, global_buf);
          }
        break;
        case itemlist_val:
          result = dstrcat(result, "%itemlist[\n");
          for (
            int k = 0;
            k < csp->sections[i].fields[j].itemlist_val.item_count;
            k++
          ) {
            result = dstrcat(result, "    %item[val:");
            if (csp->sections[i].fields[j].itemlist_val.items[k].val == NULL) {
              result = dstrcat(result, "NULL;qty:");
            } else {
              snprintf(
                global_buf, GLOBAL_BUF_SIZE,
                "\"%s\";qty:",
                csp->sections[i].fields[j].itemlist_val.items[k].val
              );
              result = dstrcat(result, global_buf);
            }
            if (csp->sections[i].fields[j].itemlist_val.items[k].qty == INT_MIN) {
              result = dstrcat(result, "NULL;weight:");
            } else {
              snprintf(
                global_buf, GLOBAL_BUF_SIZE,
                "%d;weight:",
                csp->sections[i].fields[j].itemlist_val.items[k].qty
              );
              result = dstrcat(result, global_buf);
            }
            if (csp->sections[i].fields[j].itemlist_val.items[k].weight == INT_MIN) {
              result = dstrcat(result, "NULL];\n");
            } else {
              snprintf(
                global_buf, GLOBAL_BUF_SIZE,
                "%d];\n",
                csp->sections[i].fields[j].itemlist_val.items[k].weight
              );
              result = dstrcat(result, global_buf);
            }
          }
          result = dstrcat(result, "  ];\n");
        break;
        case item_val:
          result = dstrcat(result, "    %item[val:");
          if (csp->sections[i].fields[j].item_val.val == NULL) {
            result = dstrcat(result, "NULL;qty:");
          } else {
            snprintf(
              global_buf, GLOBAL_BUF_SIZE,
              "\"%s\";qty:",
              csp->sections[i].fields[j].item_val.val
            );
            result = dstrcat(result, global_buf);
          }
          if (csp->sections[i].fields[j].item_val.qty == INT_MIN) {
            result = dstrcat(result, "NULL;weight:");
          } else {
            snprintf(
              global_buf, GLOBAL_BUF_SIZE,
              "%d;weight:",
              csp->sections[i].fields[j].item_val.qty
            );
            result = dstrcat(result, global_buf);
          }
          if (csp->sections[i].fields[j].item_val.qty == INT_MIN) {
            result = dstrcat(result, "NULL];\n");
          } else {
            snprintf(
              global_buf, GLOBAL_BUF_SIZE,
              "%d];\n",
              csp->sections[i].fields[j].item_val.weight
            );
            result = dstrcat(result, global_buf);
          }
        break;
        default:
          free(result);
          return NULL;
        break;
      }
    }
    result = dstrcat(result, "@end-section\n\n");
  }

  return result;
}
