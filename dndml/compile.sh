compile() {
 echo "gcc -c dnd_input_reader.c -Wall -std=gnu11 -g -fvar-tracking"
 gcc -c dnd_input_reader.c -Wall -std=gnu11 -g -fvar-tracking
 echo "gcc -c dnd_lexer.c -Wall -std=gnu11 -g -fvar-tracking"
 gcc -c dnd_lexer.c -Wall -std=gnu11 -g -fvar-tracking
 echo "gcc -c dnd_charsheet.c -Wall -std=gnu11 -g -fvar-tracking"
 gcc -c dnd_charsheet.c -Wall -std=gnu11 -g -fvar-tracking
 echo "gcc -c dnd_parser.c -Wall -std=gnu11 -g -fvar-tracking $@"
 gcc -c dnd_parser.c -Wall -std=gnu11 -g -fvar-tracking $@
 echo " gcc test_dnd_parser.c \
  dnd_input_reader.o  \
  dnd_lexer.o         \
  dnd_charsheet.o     \
  dnd_parser.o        \
  -o test-parser.x86 -Wall -std=gnu11 -g -fvar-tracking"
 gcc test_dnd_parser.c \
  dnd_input_reader.o  \
  dnd_lexer.o         \
  dnd_charsheet.o     \
  dnd_parser.o        \
  -o test-parser.x86 -Wall -std=gnu11 -g -fvar-tracking
}
