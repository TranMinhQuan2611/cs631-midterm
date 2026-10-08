#define _DEFAULT_SOURCE

#include "ls.h"
#include "print.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>

static Options *current_options;

static int compare_name(const void *a, const void *b)
{
    const Entry *x = a;
    const Entry *y = b;

    return strcmp(x->name, y->name);
}

static int compare_size(const void *a, const void *b)
{
    const Entry *x = a;
    const Entry *y = b;

    if (x->st.st_size < y->st.st_size)
        return 1;

    if (x->st.st_size > y->st.st_size)
        return -1;

    return strcmp(x->name, y->name);
}

static int compare_time(const void *a, const void *b)
{
    const Entry *x = a;
    const Entry *y = b;

    time_t tx;
    time_t ty;

    if (current_options->u) {
        tx = x->st.st_atime;
        ty = y->st.st_atime;
    }
    else if (current_options->c) {
        tx = x->st.st_ctime;
        ty = y->st.st_ctime;
    }
    else {
        tx = x->st.st_mtime;
        ty = y->st.st_mtime;
    }

    if (tx < ty)
        return 1;

    if (tx > ty)
        return -1;

    return strcmp(x->name, y->name);
}

static void reverse_entries(Entry *entries, size_t count)
{
    size_t i;

    for (i = 0; i < count / 2; i++) {
        Entry temp;

        temp = entries[i];
        entries[i] = entries[count - 1 - i];
        entries[count - 1 - i] = temp;
    }
}

static char *format_name(const Entry *entry, Options *opt)
{
    size_t len;
    size_t i;
    size_t pos = 0;
    char *result;

    len = strlen(entry->name) + 3;

    result = malloc(len);

    if (result == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for (i = 0; entry->name[i] != '\0'; i++) {
        unsigned char c;

        c = (unsigned char)entry->name[i];

        if (opt->q && (c < 32 || c == 127))
            result[pos++] = '?';
        else
            result[pos++] = entry->name[i];
    }

    if (opt->F) {
        if (S_ISDIR(entry->st.st_mode))
            result[pos++] = '/';
        else if (S_ISLNK(entry->st.st_mode))
            result[pos++] = '@';
        else if (S_ISFIFO(entry->st.st_mode))
            result[pos++] = '|';
        else if (S_ISSOCK(entry->st.st_mode))
            result[pos++] = '=';
        else if (entry->st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
            result[pos++] = '*';
    }

    result[pos] = '\0';

    return result;
}

void parse_options(int argc, char **argv, Options *opt, int *first)
{
    memset(opt, 0, sizeof(*opt));

    *first = 1;

    while (*first < argc &&
           argv[*first][0] == '-' &&
           argv[*first][1] != '\0') {

        const char *p;

        if (strcmp(argv[*first], "--") == 0) {
            (*first)++;
            break;
        }

        p = argv[*first] + 1;

        while (*p != '\0') {
            switch (*p) {
                case 'A':
                    opt->A = 1;
                    break;

                case 'a':
                    opt->a = 1;
                    break;

                case 'c':
                    opt->c = 1;
                    break;

                case 'd':
                    opt->d = 1;
                    break;

                case 'F':
                    opt->F = 1;
                    break;

                case 'f':
                    opt->f = 1;
                    break;

                case 'h':
                    opt->h = 1;
                    break;

                case 'i':
                    opt->i = 1;
                    break;

                case 'k':
                    opt->k = 1;
                    break;

                case 'l':
                    opt->l = 1;
                    break;

                case 'n':
                    opt->n = 1;
                    opt->l = 1;
                    break;

                case 'q':
                    opt->q = 1;
                    break;

                case 'R':
                    opt->R = 1;
                    break;

                case 'r':
                    opt->r = 1;
                    break;

                case 'S':
                    opt->S = 1;
                    break;

                case 's':
                    opt->s = 1;
                    break;

                case 't':
                    opt->t = 1;
                    break;

                case 'u':
                    opt->u = 1;
                    break;

                case 'w':
                    opt->w = 1;
                    break;

                default:
                    fprintf(stderr,
                            "ls: illegal option -- %c\n",
                            *p);
                    exit(EXIT_FAILURE);
            }

            p++;
        }

        (*first)++;
    }
}

void print_entry(Entry *entry, Options *opt)
{
    char *name;

    if (opt->i)
        printf("%llu ",
               (unsigned long long)entry->st.st_ino);

    if (opt->s)
        printf("%llu ",
               (unsigned long long)entry->st.st_blocks);

    if (opt->l) {
        print_long(entry, opt);
        return;
    }

    name = format_name(entry, opt);

    printf("%s\n", name);

    free(name);
}

void list_directory(const char *path,
                    Options *opt,
                    int level)
{
    DIR *dir;
    struct dirent *item;

    Entry *entries;
    size_t count = 0;
    size_t capacity = 32;
    size_t i;

    dir = opendir(path);

    if (dir == NULL) {
        fprintf(stderr,
                "ls: %s: %s\n",
                path,
                strerror(errno));
        return;
    }

    entries = calloc(capacity, sizeof(Entry));

    if (entries == NULL) {
        perror("calloc");
        closedir(dir);
        exit(EXIT_FAILURE);
    }

    while ((item = readdir(dir)) != NULL) {

        if (!opt->a && !opt->A &&
            item->d_name[0] == '.')
            continue;

        if (opt->A &&
            (strcmp(item->d_name, ".") == 0 ||
             strcmp(item->d_name, "..") == 0))
            continue;

        if (count == capacity) {
            capacity *= 2;

            entries = realloc(
                entries,
                capacity * sizeof(Entry)
            );

            if (entries == NULL) {
                perror("realloc");
                closedir(dir);
                exit(EXIT_FAILURE);
            }
        }

        entries[count].name = strdup(item->d_name);

        if (entries[count].name == NULL) {
            perror("strdup");
            closedir(dir);
            exit(EXIT_FAILURE);
        }

        entries[count].path =
            malloc(strlen(path) +
                   strlen(item->d_name) + 2);

        if (entries[count].path == NULL) {
            perror("malloc");
            closedir(dir);
            exit(EXIT_FAILURE);
        }

        sprintf(entries[count].path,
                "%s/%s",
                path,
                item->d_name);

        if (lstat(entries[count].path,
                   &entries[count].st) == -1) {

            free(entries[count].name);
            free(entries[count].path);
            continue;
        }

        count++;
    }

    closedir(dir);

    current_options = opt;

    if (!opt->f) {

        if (opt->S)
            qsort(entries,
                  count,
                  sizeof(Entry),
                  compare_size);

        else if (opt->t)
            qsort(entries,
                  count,
                  sizeof(Entry),
                  compare_time);

        else
            qsort(entries,
                  count,
                  sizeof(Entry),
                  compare_name);

        if (opt->r)
            reverse_entries(entries, count);
    }

    if (opt->l) {
        unsigned long long total = 0;

        for (i = 0; i < count; i++)
            total += entries[i].st.st_blocks;

        printf("total %llu\n", total);
    }

    for (i = 0; i < count; i++)
        print_entry(&entries[i], opt);

    if (opt->R) {

        for (i = 0; i < count; i++) {

            if (S_ISDIR(entries[i].st.st_mode) &&
                strcmp(entries[i].name, ".") != 0 &&
                strcmp(entries[i].name, "..") != 0) {

                printf("\n%s:\n",
                       entries[i].path);

                list_directory(
                    entries[i].path,
                    opt,
                    level + 1
                );
            }
        }
    }

    for (i = 0; i < count; i++)
        free_entry(&entries[i]);

    free(entries);
}

void list_path(const char *path,
               Options *opt,
               int level)
{
    struct stat st;
    Entry entry;

    if (lstat(path, &st) == -1) {
        fprintf(stderr,
                "ls: %s: %s\n",
                path,
                strerror(errno));
        return;
    }

    if (!S_ISDIR(st.st_mode) || opt->d) {

        entry.name = (char *)path;
        entry.path = (char *)path;
        entry.st = st;

        print_entry(&entry, opt);

        return;
    }

    if (level > 0)
        printf("\n%s:\n", path);

    list_directory(path, opt, level);
}

void free_entry(Entry *entry)
{
    free(entry->name);
    free(entry->path);
}

int main(int argc, char **argv)
{
    Options opt;
    int first;
    int i;
    int operands;

    parse_options(argc, argv, &opt, &first);

    operands = argc - first;

    if (operands == 0) {
        list_path(".", &opt, 0);
        return 0;
    }

    for (i = first; i < argc; i++) {

        if (operands > 1)
            printf("%s:\n", argv[i]);

        list_path(argv[i], &opt, 0);

        if (i < argc - 1)
            printf("\n");
    }

    return 0;
}
