#ifndef DND_INPUT_READER_H
#define DND_INPUT_READER_H

#include <stdio.h>

typedef struct input_reader {
  const char *text; // text to be parsed
  size_t pos; // position of current char in the text
  size_t text_len;
  char (*peek)(const struct input_reader *);
  void (*advance)(struct input_reader *);
  char current_char; // should be read-only outside this object's methods
} input_reader_t;

/**
 * This function does NOT pass ownership of *text to *dest. *text
 * must be freed elsewhere if it lives on the heap.
 */
void construct_input_reader(input_reader_t *dest, const char *text);

#endif // DND_INPUT_READER_H
