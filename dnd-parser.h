#include <stdio.h>

typedef struct input_reader {
  char *text; // text to be parsed
  size_t pos; // position of current char in the text
  char (*peek)(struct input_reader *);
  char (*advance)(struct input_reader *);
  int  (*has_reached_eof)(struct input_reader *);
} input_reader_t;

void construct_input_reader(
  input_reader_t *dest,
  const char *text
);
