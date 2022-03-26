#ifndef CHARSHEET_UTILS_H
#define CHARSHEET_UTILS_H

char *dnd_query_charsheet(
  const char *charsheet_id,
  const char *section_id,
  const char *field_id
);

char *cmd_dnd(int margc, char *margv[]);

#endif // CHARSHEET_UTILS_H
