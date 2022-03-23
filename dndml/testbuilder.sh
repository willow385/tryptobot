#!/bin/bash

if [ "$1" = "--dndir" ]; then
  echo "gcc -c dnd_input_reader.c -Wall -std=gnu11"
  gcc -c dnd_input_reader.c -Wall -std=gnu11
  echo "gcc test_dnd_input_reader.c dnd_input_reader.o -o test-ir.x86 -Wall -std=gnu11"
  gcc test_dnd_input_reader.c dnd_input_reader.o -o test-ir.x86 -Wall -std=gnu11
elif [ "$1" = "--dndlex" ]; then
  echo "gcc -c dnd_input_reader.c -Wall -std=gnu11"
  gcc -c dnd_input_reader.c -Wall -std=gnu11
  echo "gcc -c dnd_lexer.c -Wall -std=gnu11"
  gcc -c dnd_lexer.c -Wall -std=gnu11
  echo "gcc test_dnd_lexer.c dnd_lexer.o dnd_input_reader.o -o test-lex.x86 -Wall -std=gnu11"
  gcc test_dnd_lexer.c dnd_lexer.o dnd_input_reader.o -o test-lex.x86 -Wall -std=gnu11
elif [ "$1" == "--dndparse" ]; then
  echo "gcc -c dnd_input_reader.c -Wall -std=gnu11 -g -fvar-tracking"
  gcc -c dnd_input_reader.c -Wall -std=gnu11 -g -fvar-tracking
  echo "gcc -c dnd_lexer.c -Wall -std=gnu11 -g -fvar-tracking"
  gcc -c dnd_lexer.c -Wall -std=gnu11 -g -fvar-tracking
  echo "gcc -c dnd_charsheet.c -Wall -std=gnu11 -g -fvar-tracking $2"
  gcc -c dnd_charsheet.c -Wall -std=gnu11 -g -fvar-tracking $2
  echo "gcc -c dnd_parser.c -Wall -std=gnu11 -g -fvar-tracking $2"
  gcc -c dnd_parser.c -Wall -std=gnu11 -g -fvar-tracking $2
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
elif [ "$1" = "--clean" ]; then
  echo 'rm test*.x86'
  rm test*.x86
  echo 'rm *.o'
  rm *.o
else
  echo 'usage: ./testbuilder.sh [--dndir|--dndlex|--dndparse|--clean]'
  echo 'options:'
  echo '  --dndir: builds test for dnd_input_reader.{c,h}'
  echo '  --dndlex: builds text for dnd_lexer.{c,h}'
  echo '  --clean: removes *.x86 and *.o'
fi
