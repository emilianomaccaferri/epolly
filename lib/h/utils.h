#pragma once
#include <stddef.h>

extern void make_nonblocking(int fd);
extern int count_lines(char* bytes, size_t byte_len);
extern char** bytes_to_lines(char* bytes, size_t byte_len, int lines_num);