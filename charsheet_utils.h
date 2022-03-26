#ifndef CHARSHEET_UTILS_H
#define CHARSHEET_UTILS_H

char *dnd_query_charsheet(
  const char *charsheet_id,
  const char *section_id,
  const char *field_id
);

char *cmd_dnd(int margc, char *margv[]);

int copy_file(const char *src_path, const char *dest_path);

#endif // CHARSHEET_UTILS_H
