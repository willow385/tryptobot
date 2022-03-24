#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dnd_input_reader.h"
#include "dnd_lexer.h"
#include "dnd_charsheet.h"
#include "dnd_parser.h"

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
 gcc -c dnd_charsheet.c -Wall -std=gnu11
 gcc -c dnd_parser.c -Wall -std=gnu11
 gcc test_dnd_parser.c \
  dnd_input_reader.o  \
  dnd_lexer.o         \
  dnd_charsheet.o     \
  dnd_parser.o        \
  -o test-parser.x86 -Wall -std=gnu11
 ./test-parser.x86 [file to parse]
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

  parser_t parser;
  construct_parser(&parser, &lex, argv[1]);

  charsheet_t *charsheet = parser.parse(&parser);

  if (charsheet == NULL) {
    printf("parser.parse() returned a NULL object.\n");
    return 1;
  }

  char *output = charsheet_to_str(charsheet);

  if (output != NULL)
    printf("%s", output);
  else
    printf("Null pointer obtained\n");

  free_charsheet(charsheet);
  free(output);
  return 0;
}
