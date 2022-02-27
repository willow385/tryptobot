#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "jsmn.h"

int c_add_nums(int a, int b) {
  return a + b;
}

// copied from here https://stackoverflow.com/a/19674312
unsigned char *utf8_reverse(const unsigned char *str, int size) {
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

typedef struct command_t_vec {
  size_t size;
  command_t *commands;
} command_t_vec;

// frees the MEMBERS of *command, NOT command itself
void free_command_t_members(command_t *command) {
  free(command->command);
  free(command->syntax);
  free(command->description);
}

// transfers ownership of passed pointers to *dest
void create_command(
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
jsmntok_t *json_tokenize(char *json_string, int *token_ct) {
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

command_t_vec *load_commands(void) {
  FILE *f = fopen("commands.json", "rb");
  char *json_string;
  if (f) {
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    int x;
    fseek(f, 0, SEEK_SET);
    json_string = malloc(length+1);
    if (json_string) {
      x = fread(json_string, 1, length, f);
      json_string[x] = '\0';
    } else {
      fprintf(stderr, "Memory allocation error\n");
      return NULL;
    }
  } else {
    fprintf(stderr, "Unable to find `commands.json`\n");
    return NULL;
  }

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
  command_t_vec *result = malloc(sizeof(command_t_vec));
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
  if (!strcmp(margv[0], "%commands")) {
    command_t_vec *commands_vec = load_commands();
    if (commands_vec == NULL) {
      return strdup("Backend error");
    }
    size_t result_len;
    char *result_start = "List of commands supported by tryptobot:\n";
    result_len = strlen(result_start);
    for (int i = 0; i < commands_vec->size; i++) {
      result_len += snprintf(
        NULL, 0,
        "`%s`\n",
        commands_vec->commands[i].command
      );
    }
    char *result_end = "For more info about a specific command, "
                       "try `%cmdinfo <command>`.\n";
    result_len += strlen(result_end);
    char *result = malloc(result_len + 1);
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
  } else if (!strcmp(margv[0], "%cmdinfo")) {
    if (margc < 2) {
      return strdup(
        "Error: no command specified. "
        "Syntax is `%cmdinfo <command>`."
      );
    }
    char *queried_command = margv[1];
    command_t_vec *commands_vec = load_commands();
    command_t *result_command = NULL; // non-owning pointer
    for (int i = 0; i < commands_vec->size; i++) {
      if (!strcmp(commands_vec->commands[i].command, queried_command)) {
        result_command = commands_vec->commands + i;
      }
    }
    char *result;
    if (result_command == NULL) {
      char *err_msg_start = "Unable to find info for command `";
      char *err_msg_end = "`. Did you forget to include a leading '%'?";
      int result_len = snprintf(
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
      char *result_pt_0 = "Command syntax: `";
      char *result_pt_1 = "`\nCommand description: ";
      int result_len = snprintf(
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
  } else if (!strcmp(margv[0], "%reverse")) {
    if (margc == 2 && !strcmp(margv[1], "Ipswich")) {
      return strdup("Bolton");
    } else if (margc == 2 && !strcmp(margv[1], "ipswich")) {
      return strdup("bolton");
    } else {
      char *msg_ptr = (char *) msg + 9; // 9 == strlen("%reverse ")
      while (*msg_ptr == ' ') msg_ptr++;
      char *result = utf8_reverse(msg_ptr, strlen(msg_ptr) + 1);
      if (result == NULL) {
        return strdup("Memory allocation error");
      }
      return result;
    }
  } else {
    char *result;
    const char *err_msg = "Error: Unrecognized/malformed command `";
    const char *err_msg_end = "`.";
    size_t size = strlen(err_msg) + strlen(margv[0]) + strlen(err_msg_end) + 1;
    result = malloc(size);
    snprintf(result, size, "%s%s%s", err_msg, margv[0], err_msg_end);
    return result;
  }

  // cleanup
  for (int i = 0; i < margc-1; i++) {
    free(margv[i]);
  }
  free(margv);
}
