#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include "tryptobot.h"
#include "dstrcat.h"
#include "dndml/dnd_input_reader.h"
#include "dndml/dnd_charsheet.h"
#include "dndml/dnd_lexer.h"
#include "dndml/dnd_parser.h"
#include "charsheet_utils.h"
#include "dice.h"

#define GLOBAL_BUF_SIZE 2048
static char global_buf[GLOBAL_BUF_SIZE];

static char *get_field_list_from_section(section_t section);
static char *field_to_str(field_t field);

char *cmd_dnd(int margc, char *margv[]) {
  if (margc < 2) {
    return strdup(
      "No subcommand given.\n"
      "Supported subcommands:\n"
      "`%dnd query`\n"
      "`%dnd wtf`"
    );
  }

  char *result = NULL;
  if (!strcmp(margv[1], "query")) {
    if (margc < 3) {
      return strdup(
        "Error: no character sheet specified.\n"
        "Try `%dnd query <character sheet>`."
      );
    }
    char *charsheet_id = margv[2];
    char *section_id = NULL, *field_id = NULL;
    if (margc >= 4)
      section_id = margv[3];
    if (margc >= 5)
      field_id = margv[4];
    result = dnd_query_charsheet(charsheet_id, section_id, field_id);
    if (result != NULL) {
      return result;
    } else {
      return strdup(
        "Backend error: dnd_query_charsheet() returned null pointer"
      );
    }
  } else if (!strcmp(margv[1], "wtf")) {
    result = strdup("Most recent error:\n");
    char *err = load_file_to_str("/home/runner/tryptobot/dndml/errlog.txt");
    if (err != NULL) {
      result = dstrcat(result, err);
    } else {
      result = dstrcat(result, "(unable to load last error)");
    }
  } else {
    result = strdup("Error: Unsupported subcommand given for `%dnd`.");
  }
  return result;
}

char *dnd_query_charsheet(
  const char *charsheet_id,
  const char *section_id,
  const char *field_id
) {
  size_t path_len = 1 + strlen("charsheets/") + strlen(charsheet_id) + strlen(".dnd");
  char *canonical_path = malloc(path_len);
  sprintf(canonical_path, "charsheets/%s.dnd", charsheet_id);
  char *file_contents = load_file_to_str(canonical_path);
  if (file_contents == NULL) {
    char *msg = NULL;
    snprintf(
      global_buf, GLOBAL_BUF_SIZE,
      "error: %s: no such file or directory\n",
      canonical_path
    );
    msg = dstrcat(msg, global_buf);
    free(canonical_path);
    return msg;
  }

  input_reader_t ir;
  construct_input_reader(&ir, file_contents);

  lexer_t lex;
  construct_lexer(&lex, &ir);

  parser_t parser;
  construct_parser(&parser, &lex, canonical_path);

  charsheet_t *charsheet = parser.parse(&parser);

  if (charsheet == NULL) {
    free(canonical_path);
    free(file_contents);
    return strdup(
      "Backend error: parser.parse() returned a NULL object.\n"
      "This could mean that the character sheet file was malformed, "
      "or that there's a bug in my source code.\n"
      "For more info, do `%dnd wtf`."
    );
  }

  char *result = NULL;
  // I use `goto` here and I'm not sorry
  if (section_id != NULL) {
    for (int i = 0; i < charsheet->section_count; i++) {
      if (!strcmp(charsheet->sections[i].identifier, section_id)) {
        if (field_id != NULL) {
          for (int j = 0; j < charsheet->sections[i].field_count; j++) {
            if (!strcmp(charsheet->sections[i].fields[j].identifier, field_id)) {
              char *field_as_str = field_to_str(charsheet->sections[i].fields[j]);
              snprintf(
                global_buf, GLOBAL_BUF_SIZE,
                "Field **%s** of section **%s**:\n%s",
                field_id,
                section_id,
                field_as_str
              );
              result = strdup(global_buf);
              free(field_as_str);
              goto end_of_loop;
            }
          }
          snprintf(
            global_buf, GLOBAL_BUF_SIZE,
            "Unable to find field **%s** of section **%s**",
            field_id,
            section_id  
          );
          result = dstrcat(result, global_buf);
          goto end_of_loop;
        } else {
          char *fields_as_str = get_field_list_from_section(charsheet->sections[i]);
          snprintf(
            global_buf, GLOBAL_BUF_SIZE,
            "Fields of section **%s**:\n%s",
            section_id,
            fields_as_str
          );
          result = strdup(global_buf);
          result = dstrcat(
            result,
            "To query a specific field of a section, "
            "do `%dnd query <character sheet> <section> "
            "<field>`."
          );
          free(fields_as_str);
          goto end_of_loop;
        }
      }
    }
  } else {
    snprintf(
      global_buf, GLOBAL_BUF_SIZE,
      "Sections of character sheet **%s**:\n",
      charsheet_id
    );
    result = dstrcat(result, global_buf);
    for (int j = 0; j < charsheet->section_count; j++) {
      result = dstrcat(result, "**");
      result = dstrcat(result, charsheet->sections[j].identifier);
      result = dstrcat(result, "**\n");
    }
    result = dstrcat(
      result,
      "To list the fields of a section, do `%dnd query <character sheet> <section>`."
    );
    goto end_of_loop;
  }

  snprintf(
    global_buf, GLOBAL_BUF_SIZE,
    "Unable to find section **%s** in sheet **%s**",
    section_id,
    charsheet_id
  );

end_of_loop:
  free(canonical_path);
  free_charsheet(charsheet);
  return result;
}

static char *get_field_list_from_section(section_t section) {
  char *result = NULL;
  for (int i = 0; i < section.field_count; i++) {
    char *field_as_str = field_to_str(section.fields[i]);
    snprintf(
      global_buf,
      GLOBAL_BUF_SIZE,
      "**%s**: %s\n",
      section.fields[i].identifier,
      field_as_str
    );
    result = dstrcat(result, global_buf);
    free(field_as_str);
    if (section.fields[i].type == itemlist_val)
      result = dstrcat(result, "\n");
  }
  return result;
}

static char *field_to_str(field_t field) {
  char *result = NULL;
  switch (field.type) {
    case stat_val:
      if (field.stat_val.ability != INT_MIN) {
        snprintf(
          global_buf, GLOBAL_BUF_SIZE,
          "ability: %d",
          field.stat_val.ability
        );
        result = dstrcat(result, global_buf);
      }
      if (field.stat_val.mod != INT_MIN) {
        if (field.stat_val.ability != INT_MIN)
          result = dstrcat(result, "\n");
        snprintf(
          global_buf, GLOBAL_BUF_SIZE,
          "modifier: %d",
          field.stat_val.mod
        );
        result = dstrcat(result, global_buf);
      }
    break;
    case string_val:
      if (field.string_val != NULL) {
        result = dstrcat(result, field.string_val);
      }
    break;
    case int_val:
      if (field.int_val != INT_MIN) {
        snprintf(global_buf, GLOBAL_BUF_SIZE, "%d", field.int_val);
        result = dstrcat(result, global_buf);
      }
    break;
    case dice_val:
      if (field.dice_val.value != INT_MIN) {
        snprintf(
          global_buf, GLOBAL_BUF_SIZE,
          "%dd%d+%d",
          field.dice_val.dice_ct,
          field.dice_val.faces,
          field.dice_val.modifier
        );
        result = dstrcat(result, global_buf);
      }
    break;
    case deathsave_val:
      if (field.deathsave_val.succ != INT_MIN) {
        snprintf(
          global_buf, GLOBAL_BUF_SIZE,
          "succeeded: %d",
          field.deathsave_val.succ
        );
        result = dstrcat(result, global_buf);
      }
      if (field.deathsave_val.fail != INT_MIN) {
        if (field.deathsave_val.succ != INT_MIN)
          result = dstrcat(result, "\n");
        snprintf(
          global_buf, GLOBAL_BUF_SIZE,
          "failed: %d",
          field.deathsave_val.fail
        );
        result = dstrcat(result, global_buf);
      }
    break;
    case itemlist_val:
      result = dstrcat(result, "Contents:");
      if (field.itemlist_val.items != NULL) {
        for (int i = 0; i < field.itemlist_val.item_count; i++) {
          if (field.itemlist_val.items[i].val != NULL) {
            snprintf(
              global_buf, GLOBAL_BUF_SIZE,
              "\nItem %d: ", i+1
            );
            result = dstrcat(result, global_buf);
            result = dstrcat(result, field.itemlist_val.items[i].val);
          }
          if (field.itemlist_val.items[i].qty != INT_MIN) {
            result = dstrcat(result, "\n  Quantity: ");
            snprintf(
              global_buf, GLOBAL_BUF_SIZE,
              "%d", field.itemlist_val.items[i].qty
            );
            result = dstrcat(result, global_buf);
          }
          if (!isnan(field.itemlist_val.items[i].weight)) {
            result = dstrcat(result, "\n  Weight: ");
            snprintf(
              global_buf, GLOBAL_BUF_SIZE,
              "%.2f lb", field.itemlist_val.items[i].weight
            );
            result = dstrcat(result, global_buf);
          }
        }
      }
    break;
    case item_val:
      result = dstrcat(result, "Item: ");
      if (field.item_val.val != NULL) {
        result = dstrcat(result, field.item_val.val);
      }
      if (field.item_val.qty != INT_MIN) {
        result = dstrcat(result, "\nQuantity: ");
        snprintf(global_buf, GLOBAL_BUF_SIZE, "%d", field.item_val.qty);
        result = dstrcat(result, global_buf);
      }
      if (!isnan(field.item_val.weight)) {
        result = dstrcat(result, "\nWeight: ");
        snprintf(global_buf, GLOBAL_BUF_SIZE, "%.2f lb", field.item_val.weight);
        result = dstrcat(result, global_buf);
      }
    break;
    default:
      result = dstrcat(result, "Backend error: Invalid value for `field_t.type`");
    break;
  }
  return result;
}
