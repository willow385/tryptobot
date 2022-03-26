#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dstrcat.h"

#ifdef DEBUG_LVL
  #if DEBUG_LVL == 0
    #define DEBUG1(x) ;
    #define DEBUG2(x) ;
  #elif DEBUG_LVL == 1
    #define DEBUG1(x) x;
    #define DEBUG2(x) ;
  #elif DEBUG_LVL == 2
    #define DEBUG1(x) x;
    #define DEBUG2(x) x;
  #endif
#else
  #warning DEBUG_LVL undefined; defaulting to 0
  #define DEBUG1(x) ;
  #define DEBUG2(x) ;
#endif

// dynamically appends a string to another heap-allocated string
char *dstrcat(char *dest, const char *str_to_append) {
  DEBUG1(
    fprintf(stderr, "~~ Calling dstrcat(");
    if (dest) {
      fprintf(stderr, "\"%s\", \"%s\").\n", dest, str_to_append);
    } else {
      fprintf(stderr, "%p, \"%s\").\n", dest, str_to_append);
    }
    fprintf(stderr, "~~ ============== ~~\n");
  );
  size_t bufsize;
  if (dest != NULL)
    bufsize = 1 + snprintf(NULL, 0, "%s", str_to_append) + strlen(dest);
  else
    bufsize = 1 + snprintf(NULL, 0, "%s", str_to_append);
  DEBUG2(
    fprintf(
      stderr, "~~   Allocating %lu chars to dest.\n", bufsize
    );
    fprintf(
      stderr,
      "~~   strlen(dest): %lu\n",
      dest == NULL ? 0 : strlen(dest)
    );
  );
  _Bool should_not_strlen_dest = dest == NULL;
  dest = realloc(dest, bufsize);
  if (should_not_strlen_dest)
    sprintf(dest, "%s", str_to_append);
  else
    sprintf(dest + strlen(dest), "%s", str_to_append);

  DEBUG2(fprintf(stderr, "~~   Result: \"%s\"\n", dest));
  return dest;
}
