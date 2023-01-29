#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stddef.h>

char* file_entry_type_to_str(unsigned char type);
char* get_file_contents(char const * const filename);
void print_file_contents(char const * const filename);
void write_to_file(char const * const filename, char const * const text);
bool dir_exists(char const * const dirname);
void create_dir_if_not_exists(char const * const dirname);
void print_dir(char const* const dirname);
void print_dir_with_ownership_info(char const* const dirname);
void print_dir_nums(char const* const dirname);
void get_current_time_str(char * const string, size_t max_size);

#endif // !UTILS_H

