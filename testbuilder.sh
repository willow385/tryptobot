#!/bin/bash

if [ "$1" = "--dndir" ]; then
  gcc -c dnd_input_reader.c -Wall
  gcc test_dnd_input_reader.c dnd_input_reader.o -o test-ir.x86 -Wall
elif [ "$1" = "--dndlex" ]; then
  gcc -c dnd_input_reader.c -Wall
  gcc -c dnd_lexer.c -Wall
  gcc test_dnd_lexer.c dnd_lexer.o dnd_input_reader.o -o test-lex.x86 -Wall
elif [ "$1" = "--clean" ]; then
  rm test*.x86
  rm *.o
else
  echo 'usage: ./testbuilder.sh [--dndir|--clean]'
  echo 'options:'
  echo '  --dndir: builds test for dnd_input_reader.{c,h}'
  echo '  --dndlex: builds text for dnd_lexer.{c,h}'
  echo '  --clean: removes *.x86 and *.o'
fi
