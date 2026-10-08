#ifndef PRINT_H
#define PRINT_H

#include "ls.h"

void print_long(Entry *entry, Options *opt);
void print_permissions(mode_t mode);
void print_size(off_t size, Options *opt);
void print_time(time_t value);

#endif
