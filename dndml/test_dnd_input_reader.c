#include <stdio.h>
#include "dnd_input_reader.h"

/*
 gcc -c dnd_input_reader.c -Wall -std=gnu11
 gcc test_dnd_input_reader.c dnd_input_reader.o -o test-ir.x86 -Wall -std=gnu11
 ./test-ir.x86 [string to test against]
 */
int main(int argc, char *argv[]) {
  const char *test_string = "Test string";
  if (argc > 1) {
    test_string = argv[1];
  }

  input_reader_t ir;
  construct_input_reader(&ir, test_string);

  printf("Number of chars to read: %lu\n", ir.text_len);
  while (ir.current_char) {
    printf(
      "Current char: %c; next char: %c; pos: %lu\n",
      ir.current_char, ir.peek(&ir), ir.pos
    );
    ir.advance(&ir);
  }

  return 0;
}
