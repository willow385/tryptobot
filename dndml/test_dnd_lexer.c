#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dnd_input_reader.h"
#include "dnd_lexer.h"

// copied straight from tryptobot.c; quick hack but it works
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

/*
 gcc -c dnd_input_reader.c -Wall -std=gnu11
 gcc -c dnd_lexer.c -Wall -std=gnu11
 gcc test_dnd_lexer.c dnd_input_reader.o dnd_lexer.o -o test-lex.x86 -Wall -std=gnu11
 ./test-lex.x86 [file to lex]
 */
int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("error: no file passed\n");
    return 1;
  }

  char *file_contents = load_file_to_str(argv[1]);
  if (file_contents == NULL) {
    printf("error: %s: no such file or directory\n", argv[1]);
    return 1;
  }

  input_reader_t ir;
  construct_input_reader(&ir, file_contents);

  lexer_t lex;
  construct_lexer(&lex, &ir);

  printf("Number of chars to read: %lu\n", ir.text_len);
  token_t current_token;
  do {
    current_token = lex.get_next_token(&lex);
    char *token_str = strndup(
      current_token.src_text + current_token.start,
      current_token.end - current_token.start
    );
    printf("Token: \"%s\"; type: %d\n", token_str, current_token.type);
    free(token_str);
  } while (current_token.type != eof);

  free(file_contents);
  return 0;
}
