#!/bin/bash

if [ "$1" = "--dndir" ]; then
  gcc -c dnd_input_reader.c -Wall
  gcc test_dnd_input_reader.c dnd_input_reader.o -o test.x86 -Wall
elif [ "$1" == "--clean" ]; then
  rm test.x86
  rm *.o
else
  echo 'usage: ./testbuilder.sh [--dndir|--clean]'
  echo 'options:'
  echo '  --dndir: builds test for dnd_input_reader.{c,h}'
  echo '  --clean: removes test.x86 and object files'
fi
