#include <stdio.h>
#include <string.h>
#include "dnd_input_reader.h"

static char input_reader_peek(const input_reader_t *this) {
  if (this->pos + 1 >= this->text_len) {
    return '\0';
  } else {
    return this->text[this->pos + 1];
  }
}

static void input_reader_advance(input_reader_t *this) {
  this->pos++;
  if (this->pos > this->text_len - 1) {
    this->current_char = '\0';
  } else {
    this->current_char = this->text[this->pos];
  }
}

void construct_input_reader(
  input_reader_t *dest,
  const char *text
) {
  dest->text = text;
  dest->pos = 0;
  dest->text_len = strlen(dest->text);
  dest->current_char = dest->text[0];
  dest->peek = &input_reader_peek;
  dest->advance = &input_reader_advance;
}
