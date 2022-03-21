"dndml" stands for "Dungeons and Dragons Markup
Language". It is a very simple DSL for representing
DnD character sheets. The file extension is ".dnd".

dndml is structured using reserved words that start
with '@': "@section", "@field", and "@end-section".
It is divided into *sections*, which are themselves
divided into *fields*, which have types and values.
Both sections and fields have identifiers. Field
values have their types specified explicitly; they
consist of '%', followed by a typename, followed
by a value enclosed in square brackets. The syntax
of the values of a given type are a feature of that
type.

Additionally, any value can be NULL.

Identifiers must start with '_' or a letter and can
contain any sequence of letters, '_', or '-', with
the exception that an identifier cannot be the word
"NULL" in all caps.

Comments begin with "~~" and end with a newline.
They can go anywhere outside a string literal.

An example section of a *.dnd file might look like
the following:

@section example-section:
  @field foo: %string["bar"];
  @field baz: %int[-1337];
  @field nullable: %string[NULL];

  ~~ %stat type is for storing wis, cha, dex, etc.
  @field quux: %stat[ability:9;  mod:-1];

  @field spam: %itemlist[
    %item[val:"eggs";qty:NULL;weight:1];
    %item[val:"toast";qty:2;weight:0];
    %item[val:"coffee";qty:1;weight:1];
  ];
@end-section
