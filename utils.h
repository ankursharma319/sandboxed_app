#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>

char* file_entry_type_to_str(unsigned char type);
void write_to_file(char const * const filename, char const * const text);
bool dir_exists(char const * const dirname);
void create_dir_if_not_exists(char const * const dirname);
void print_dir(char const* const dirname);

#endif // !UTILS_H

