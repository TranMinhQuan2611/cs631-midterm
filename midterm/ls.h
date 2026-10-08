#ifndef LS_H
#define LS_H

#include <sys/types.h>
#include <sys/stat.h>

typedef struct {
    int A, a, c, d, F, f, h, i, k, l, n, q, R, r, S, s, t, u, w;
} Options;

typedef struct {
    char *name;
    char *path;
    struct stat st;
} Entry;

void parse_options(int argc, char **argv, Options *opt, int *first);
void list_path(const char *path, Options *opt, int level);
void list_directory(const char *path, Options *opt, int level);
void print_entry(Entry *entry, Options *opt);
void free_entry(Entry *entry);

#endif
