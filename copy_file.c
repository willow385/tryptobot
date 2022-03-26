#include <stdio.h>
#include "copy_file.h"

#define BUF_SIZE 0x10000LU
static char buf[BUF_SIZE];

int copy_file(const char *src_path, const char *dest_path) {
  FILE *src, *dst;
  size_t chars_read_in, chars_written_out;
  src = fopen(src_path, "rb");
  if (src == NULL) return -1;
  dst = fopen(dest_path, "wb+");
  if (dst < 0) return -1;
  int result = 0;
  while (1) {
    chars_read_in = fread(buf, 1, BUF_SIZE, src);
    if (chars_read_in == 0) break;
    result += (chars_written_out =
      fwrite(buf, 1, chars_read_in, dst));
    if (chars_written_out == 0) break;
  }
  fclose(src);
  fclose(dst);
  return result;
}
