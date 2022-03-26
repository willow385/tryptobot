# tryptobot

## Notable files in this repository:

`./main.py`:
is run when tryptobot is restarted. This script recompiles the backend into a shared object file (`./libtryptobot.so`), dynamically loads it, connects to the Discord API, handles commands that can't be feasibly implemented in C (i.e., uploading or downloading files), serves a small static webpage, and passes any other commands (the overwhelming majority of commands) to the function `handle_message()` in `./tryptobot.c`.

`./index.html`: a very simple webpage which tryptobot serves while it's running, accessible at [tryptobot.dantefalzone.repl.co](https://tryptobot.dantefalzone.repl.co/).

`./tryptobot.c`:
the main component of tryptobot's backend. This file implements the majority of tryptobot's commands, as well as the function `load_file_to_str()` which loads a file's contents into a dynamically allocated string and has proven very useful. TODO: move `load_file_to_str()`'s implementation into its own *.c file.

`./tryptobot.h`: header file which declares the functions `handle_message()` and `load_file_to_str()`.

`./copy_file.{c,h}`: defines function `copy_file()` which copies one file to another without calling `{m,c,re}alloc()`.

`./jsmn.h`: the [jsmn library](https://github.com/zserge/jsmn/blob/master/jsmn.h), a header-only library for tokenizing JSON, written by Serge Zaitsev.

`./commands.json`: a list of the commands supported by tryptobot.

`./dice.h`: header file defining the `diceroll_t` datatype, which is a struct that represents a diceroll (for example, "rolling 2d20 with a -1 modifier giving the result 7" would be represented as `(diceroll_t){ .dice_ct=2, .faces=20, .modifier=-1, .value=7}`). TODO: move the functions for handling/manipulating `diceroll_t`s into their own file, like `dice.c`.

`./lastroll.txt`: file whither the most recent `diceroll_t` to be obtained from the `%roll` or `%reroll` commands is serialized.

`./dstrcat.c`: this contains the function `dstrcat()` ('d' being short for "dynamic"), which takes a NULL or heap-allocated `char *` and a `const char *`, and appends the `const char *` to the end of the string pointed to by the `char *`, `realloc()`ing it as needed to hold the extra `char`s and returning a pointer to the `realloc()`ed string. This is a function that's very broadly useful, and is called many times throughout tryptobot's backend.

`./dstrcat.h`: headerfile for `./dstrcat.c`.

`./charsheet_utils.{c,h}`: this contains functions for serializing a `charsheet_t` into a human-readable format, including `cmd_dnd()` which is called by `handle_message()` when someone in the Discord server uses the `%dnd` command.

`./dndml/`: this directory contains source, header, and object files for working with dndml (DnD Markup Language), most notably for serializing and deserializing character sheets written in dndml (located in `./charsheets/`). In addition to the code for serializing and deserializing dndml, it contains some simple tests for that code, so it can be tested separately from the rest of tryptobot's backend.

`./dndml/dnd_*.{c,h}`: source code for the parser (for deserializing dndml) and for functions for serializing a `charsheet_t` back into dndml. `charsheet_t` and the types from which it is composed are defined in `dnd_charsheet.h`. The parser and its component types are designed with an object-oriented interface (function pointers as struct members) and "constructor" functions for initializing them to a valid state.

`./dndml/build.sh`: build script for the tests. Run the script without any args for more info.

`./dndml/errlog.txt`: file where the most recent backend error message is stored (so it can be retrieved via `%dnd wtf` from the Discord server).

`./charsheets/`: DnD character sheets written in dndml are stored here. TODO: create a "`backups`" subdirectory to which character sheets are copied before they are modified via `%dnd` subcommands.
