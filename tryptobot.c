#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "jsmn.h"

// copied from here https://stackoverflow.com/a/19674312
static unsigned char *utf8_reverse(const unsigned char *str, int size) {
  unsigned char *ret = calloc(size, sizeof(unsigned char*));
  int ret_size = 0;
  int pos = size - 2;
  int char_size = 0;

  if (str == NULL) {
    fprintf(stderr, "failed to allocate memory.\n");
    return NULL;
  }

  while (pos > -1) {

    if (str[pos] < 0x80) {
      char_size = 1;
    } else if (pos > 0 && str[pos - 1] > 0xC1 && str[pos - 1] < 0xE0) {
      char_size = 2;
    } else if (pos > 1 && str[pos - 2] > 0xDF && str[pos - 2] < 0xF0) {
      char_size = 3;
    } else if (pos > 2 && str[pos - 3] > 0xEF && str[pos - 3] < 0xF5) {
      char_size = 4;
    } else {
      char_size = 1;
    }

    pos -= char_size;
    memcpy(ret + ret_size, str + pos + 1, char_size);
    ret_size += char_size;
  }    

  ret[ret_size] = '\0';

  return ret;
}

typedef struct command_t {
  char *command;
  char *syntax;
  char *description;
} command_t;

typedef struct command_vec_t {
  size_t size;
  command_t *commands;
} command_vec_t;

// frees the MEMBERS of *command, NOT command itself
static void free_command_t_members(command_t *command) {
  free(command->command);
  free(command->syntax);
  free(command->description);
}

// transfers ownership of passed pointers to *dest
static void create_command(
  command_t *dest,
  char *command,
  char *syntax,
  char *description
) {
  dest->command = command;
  dest->syntax = syntax;
  dest->description = description;
}

// number of tokens is stored in *token_ct
static jsmntok_t *json_tokenize(char *json_string, int *token_ct) {
  jsmn_parser p;
  jsmn_init(&p);
  int json_len = strlen(json_string);
  *token_ct = jsmn_parse(&p, json_string, json_len, NULL, INT_MAX);
  jsmntok_t *tokens = malloc(*token_ct * sizeof(jsmntok_t));
  jsmn_init(&p); // do not delete this line!
  *token_ct = jsmn_parse(
    &p,
    json_string,
    json_len,
    tokens,
    *token_ct
  );

  if (*token_ct < 0) {
    switch (*token_ct) {
      case JSMN_ERROR_INVAL:
        fprintf(stderr, "Invalid JSON: %s\n", json_string);
      break;
      case JSMN_ERROR_NOMEM:
        fprintf(stderr, "JSON too large, allocate more memory!\n");
      break;
      case JSMN_ERROR_PART:
        fprintf(stderr, "JSON is too short: %s\n", json_string);
      break;
      default:
        fprintf(stderr, "Failed to parse JSON: %d\n", *token_ct);
    }
    return NULL;
  }

  /* Assume the top-level element is an object */
  if (*token_ct < 1 || tokens[0].type != JSMN_OBJECT) {
    fprintf(stderr, "Object expected\n");
    return NULL;
  }

  return tokens;
}

static char *load_file_to_str(const char *filename) {
  FILE *f = fopen(filename, "rb");
  char *result;
  if (f) {
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    int x;
    fseek(f, 0, SEEK_SET);
    result = malloc(length+1);
    if (result) {
      x = fread(result, 1, length, f);
      result[x] = '\0';
    } else {
      fprintf(stderr, "Memory allocation error\n");
      fclose(f);
      return NULL;
    }
  } else {
    fprintf(stderr, "Unable to find `%s`\n", filename);
    return NULL;
  }
  fclose(f);
  return result;
}

static command_vec_t *load_commands(void) {
  char *json_string = load_file_to_str("commands.json");

  int token_ct;
  jsmntok_t *tokens = json_tokenize(json_string, &token_ct);

  // find the number of commands
  int command_ct = 0;
  char *json_array = strndup(
    json_string + tokens[2].start,
    tokens[2].end - tokens[2].start
  );
  for (int i = 0; json_array[i]; i++) {
    if (json_array[i] == '{') command_ct++;
  }

  // get the data from the JSON and return it
  command_vec_t *result = malloc(sizeof(command_vec_t));
  result->size = command_ct;
  result->commands = malloc(result->size * sizeof(command_t));
  for (int i = 0; i < command_ct; i++) {
    create_command(
      result->commands + i,
      strndup(
        json_string + tokens[5+7*i].start,
        tokens[5+7*i].end - tokens[5+7*i].start
      ),
      strndup(
        json_string + tokens[7+7*i].start,
        tokens[7+7*i].end - tokens[7+7*i].start
      ),
      strndup(
        json_string + tokens[9+7*i].start,
        tokens[9+7*i].end - tokens[9+7*i].start
      )
    );
  }
  free(tokens);
  free(json_string);
  return result;
}

typedef struct diceroll_t {
  int dice_ct, faces, value;
} diceroll_t;

static diceroll_t load_last_diceroll(void) {
  char *last_diceroll_str = load_file_to_str("lastroll.txt");
  if (last_diceroll_str) {
    diceroll_t result;
    sscanf(
      last_diceroll_str,
      "dice:%dd%d;val:%d;",
      &result.dice_ct, &result.faces, &result.value
    );
    return result;
  } else {
    fprintf(stderr, "Unable to read last dice roll\n");
    return (diceroll_t){-1, -1, -1};
  }
}

static void save_diceroll(diceroll_t diceroll) {
  FILE *f = fopen("lastroll.txt", "w+");
  if (f) {
    fprintf(
      f, "dice:%dd%d;val:%d;",
      diceroll.dice_ct, diceroll.faces, diceroll.value
    );
    fclose(f);
  } else {
    fprintf(stderr, "Unable to write to `lastroll.txt`\n");
  }
}

static int random_int(int min, int max){
  return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

static diceroll_t roll_dice(int dice_ct, int faces) {
  diceroll_t result;
  result.dice_ct = dice_ct;
  result.faces = faces;
  result.value = 0;
  for (int i = 0; i < dice_ct; i++) {
    result.value += random_int(1, faces);
  }
  return result;
}

static char *cmd_commands(int margc, char **margv, int *error) {
  char *result;
  *error = 0;
  command_vec_t *commands_vec = load_commands();
  if (commands_vec == NULL) {
    result = strdup("Backend error");
    *error = 1;
    return result;
  }
  size_t result_len;
  const char *result_start = "List of commands supported by tryptobot:\n";
  result_len = strlen(result_start);
  for (int i = 0; i < commands_vec->size; i++) {
    result_len += snprintf(
      NULL, 0,
      "`%s`\n",
      commands_vec->commands[i].command
    );
  }
  const char *result_end = "For more info about a specific command, "
                           "try `%cmdinfo <command>`.\n";
  result_len += strlen(result_end);
  result = malloc(result_len + 1);
  strcpy(result, result_start);
  for (int i = 0; i < commands_vec->size; i++) {
    sprintf(
      result + strlen(result),
      "`%s`\n",
      commands_vec->commands[i].command
    );
  }
  strcat(result, result_end);
  for (int i = 0; i < commands_vec->size; i++)
    free_command_t_members(commands_vec->commands + i);
  free(commands_vec->commands);
  free(commands_vec);
  return result;
}

static char *cmd_cmdinfo(int margc, char **margv, int *error) {
  char *result;
  *error = 0; 
  if (margc < 2) {
    result = strdup(
      "Error: no command specified. "
      "Syntax is `%cmdinfo <command>`."
    );
    *error = 1;
    return result;
  }
  const char *queried_command = margv[1];
  command_vec_t *commands_vec = load_commands();
  command_t *result_command = NULL; // non-owning pointer
  for (int i = 0; i < commands_vec->size; i++) {
    if (!strcmp(commands_vec->commands[i].command, queried_command)) {
      result_command = commands_vec->commands + i;
    }
  }
  if (result_command == NULL) {
    const char *err_msg_start = "Unable to find info for command `";
    const char *err_msg_end = "`. Did you forget to include a leading '%'?";
    size_t result_len = snprintf(
      NULL, 0,
      "%s%s%s",
      err_msg_start, queried_command, err_msg_end
    );
    result = malloc(result_len + 1);
    sprintf(
      result,
      "%s%s%s",
      err_msg_start, queried_command, err_msg_end
    );
  } else {
    const char *result_pt_0 = "Command syntax: `";
    const char *result_pt_1 = "`\nCommand description: ";
    size_t result_len = snprintf(
      NULL, 0,
      "%s%s%s%s",
      result_pt_0, result_command->syntax,
      result_pt_1, result_command->description
    );
    result = malloc(result_len + 1);
    sprintf(
      result,
      "%s%s%s%s",
      result_pt_0, result_command->syntax,
      result_pt_1, result_command->description
    );
  }
  for (int i = 0; i < commands_vec->size; i++)
    free_command_t_members(commands_vec->commands + i);
  free(commands_vec->commands);
  free(commands_vec);
  return result;
}

static char *cmd_reverse(
  int margc,
  char **margv,
  const char *msg, // should be pointer passed to handle_message as msg
  int *error
) {
  char *result;
  *error = 0;
  if (margc == 2 && !strcmp(margv[1], "Ipswich")) {
    result = strdup("Bolton");
  } else if (margc == 2 && !strcmp(margv[1], "ipswich")) {
    result = strdup("bolton");
  } else {
    char *msg_ptr = (char *) msg + 9; // 9 == strlen("%reverse ")
    while (*msg_ptr == ' ') msg_ptr++;
    result = utf8_reverse(msg_ptr, strlen(msg_ptr) + 1);
    if (result == NULL) {
      result = strdup("Memory allocation error");
      *error = 1;
    }
  }
  return result;
}

/**
* Checks if diceroll_str is valid dice syntax. If an
* uppercase 'D' is present in diceroll_str,  it will
* be replaced with a lowercase 'd',  hence why it is
* passed  as  a  char *  instead of a  const char *.
* Returns 1 if diceroll_str is valid, and 0 if it is
* not.
* ==================================================
* TODO implement modifiers (e.g. "2d20+3")
*/
static int is_valid_diceroll_str(char *diceroll_str) {
  int d_ct = 0; // number of instances of 'd' or 'D'
  int result = 1;
  for (int i = 0; diceroll_str[i]; i++) {
    if (diceroll_str[i] == 'd') {
      d_ct++;
    } else if (diceroll_str[i] == 'D') {
      diceroll_str[i] = 'd';
      d_ct++;
    } else if (!isdigit(diceroll_str[i])) {
      result = 0;
    }
  }
  if (diceroll_str[0] == 'd') result = 0;
  if (d_ct != 1) result = 0;
  return result;
}

static char *cmd_roll(int margc, char **margv, int *error) {
  char *result;
  *error = 0;

  if (margc < 2) {
    result = strdup("Error: Roll what?");
    *error = 1;
    return result;
  }

  // check if the dice string is valid
  if (!is_valid_diceroll_str(margv[1])) {
    const char *err_msg = "Syntax error: `\"";
    const char *err_msg_end = "\"` is not valid dice notation.";
    size_t result_len = snprintf(
      NULL, 0, "%s%s%s",
      err_msg, margv[1], err_msg_end
    );
    result = malloc(result_len+1);
    sprintf(result, "%s%s%s", err_msg, margv[1], err_msg_end);
    *error = 1;
    return result;
  }

  // dice string has been validated, now we roll the dice
  int dice_ct, faces;
  int vals_scanned  = sscanf(margv[1], "%dd%d", &dice_ct, &faces);
  if (faces < 1 || vals_scanned != 2) {
    const char *err_msg_start = "Error: Invalid dice: ";
    size_t result_len = snprintf(
      NULL, 0,
      "%s%s",
      err_msg_start, margv[1]
    );
    result = malloc(result_len+1);
    sprintf(result, "%s%s", err_msg_start, margv[1]);
    *error = 1;
    return result;
  }
  srand(time(NULL));
  diceroll_t dice_roll = roll_dice(dice_ct, faces);
  size_t result_len = snprintf(
    NULL, 0,
    "Result of rolling %dd%d: %d",
    dice_roll.dice_ct, dice_roll.faces, dice_roll.value
  );
  result = malloc(result_len+1);
  sprintf(
    result,
    "Result of rolling %dd%d: %d",
    dice_roll.dice_ct, dice_roll.faces, dice_roll.value
  );
  save_diceroll(dice_roll);
  return result;
}

static char *cmd_reroll(int margc, char **margv, int *error) {
  char *result;
  *error = 0;

  diceroll_t last_roll = load_last_diceroll();
  if (last_roll.value == -1) {
    result = strdup("Backend error");
    *error = 1;
    return result;
  }
  srand(time(NULL));
  diceroll_t new_roll = roll_dice(last_roll.dice_ct, last_roll.faces);
  size_t result_len = snprintf(
    NULL, 0,
    "Result of rolling %dd%d: %d",
    new_roll.dice_ct, new_roll.faces, new_roll.value
  );
  result = malloc(result_len+1);
  sprintf(
    result,
    "Result of rolling %dd%d: %d",
    new_roll.dice_ct, new_roll.faces, new_roll.value
  );
  save_diceroll(new_roll);
  return result;
}

// this function is called from main.py and handles most commands
char *handle_message(const char *msg) {
  // "m" is for "message"
  int margc = 1;
  char **margv;

  // count args
  for (int i = 0; msg[i];) {
    if (msg[i] == ' ') {
      margc++;
      while (msg[i] == ' ') i++;
    } else {
      i++;
    }
  }

  char *msg_copy = strdup(msg);
  margv = malloc(margc * sizeof(char *));
  char *token = strtok(msg_copy, " ");
  for (int i = 0; token != NULL; i++) {
    margv[i] = strdup(token);
    token = strtok(NULL, " ");
  }
  free(msg_copy);

  // process command
  char *result;
  int error;
  if (!strcmp(margv[0], "%commands")) {
    result = cmd_commands(margc, margv, &error);
    if (error) goto cleanup;
  } else if (!strcmp(margv[0], "%cmdinfo")) {
    result = cmd_cmdinfo(margc, margv, &error);
    if (error) goto cleanup;
  } else if (!strcmp(margv[0], "%reverse")) {
    result = cmd_reverse(margc, margv, msg, &error);
    if (error) goto cleanup;
  } else if (!strcmp(margv[0], "%roll")) {
    result = cmd_roll(margc, margv, &error);
    if (error) goto cleanup;
  } else if (!strcmp(margv[0], "%reroll")) {
    result = cmd_reroll(margc, margv, &error);
    if (error) goto cleanup;
  } else {
    const char *err_msg = "Error: Unrecognized/malformed command `";
    const char *err_msg_end = "`.";
    size_t size = strlen(err_msg) + strlen(margv[0]) + strlen(err_msg_end) + 1;
    result = malloc(size);
    snprintf(result, size, "%s%s%s", err_msg, margv[0], err_msg_end);
  }

cleanup:
  for (int i = 0; i < margc-1; i++) {
    free(margv[i]);
  }
  free(margv);
  return result;
}
