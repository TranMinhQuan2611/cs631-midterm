#define _DEFAULT_SOURCE

#include "print.h"

#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <unistd.h>

void print_permissions(mode_t mode)
{
    char type;

    if (S_ISDIR(mode))
        type = 'd';
    else if (S_ISLNK(mode))
        type = 'l';
    else if (S_ISCHR(mode))
        type = 'c';
    else if (S_ISBLK(mode))
        type = 'b';
    else if (S_ISFIFO(mode))
        type = 'p';
    else if (S_ISSOCK(mode))
        type = 's';
    else
        type = '-';

    printf("%c", type);

    printf("%c", mode & S_IRUSR ? 'r' : '-');
    printf("%c", mode & S_IWUSR ? 'w' : '-');
    printf("%c", mode & S_IXUSR ? 'x' : '-');

    printf("%c", mode & S_IRGRP ? 'r' : '-');
    printf("%c", mode & S_IWGRP ? 'w' : '-');
    printf("%c", mode & S_IXGRP ? 'x' : '-');

    printf("%c", mode & S_IROTH ? 'r' : '-');
    printf("%c", mode & S_IWOTH ? 'w' : '-');
    printf("%c", mode & S_IXOTH ? 'x' : '-');
}

void print_size(off_t size, Options *opt)
{
    if (opt->h) {
        double value = (double)size;
        const char *units[] = {
            "B", "K", "M", "G", "T"
        };
        int unit = 0;

        while (value >= 1024.0 && unit < 4) {
            value /= 1024.0;
            unit++;
        }

        if (unit == 0)
            printf("%lld%s",
                   (long long)size,
                   units[unit]);
        else
            printf("%.1f%s",
                   value,
                   units[unit]);
    }
    else if (opt->k) {
        printf("%lld",
               ((long long)size + 1023) / 1024);
    }
    else {
        printf("%lld", (long long)size);
    }
}

void print_time(time_t value)
{
    char buffer[64];
    struct tm *tm_info;

    tm_info = localtime(&value);

    strftime(buffer,
             sizeof(buffer),
             "%b %e %H:%M",
             tm_info);

    printf("%s", buffer);
}

void print_long(Entry *entry, Options *opt)
{
    struct passwd *pw;
    struct group *gr;
    time_t timestamp;

    print_permissions(entry->st.st_mode);

    printf(" %lu ",
           (unsigned long)entry->st.st_nlink);

    if (opt->n) {
        printf("%u %u ",
               (unsigned)entry->st.st_uid,
               (unsigned)entry->st.st_gid);
    }
    else {
        pw = getpwuid(entry->st.st_uid);
        gr = getgrgid(entry->st.st_gid);

        printf("%-8s %-8s ",
               pw ? pw->pw_name : "?",
               gr ? gr->gr_name : "?");
    }

    print_size(entry->st.st_size, opt);

    printf(" ");

    if (opt->u)
        timestamp = entry->st.st_atime;
    else if (opt->c)
        timestamp = entry->st.st_ctime;
    else
        timestamp = entry->st.st_mtime;

    print_time(timestamp);

    printf(" %s", entry->name);

    if (S_ISLNK(entry->st.st_mode)) {

        char target[4096];
        ssize_t length;

        length = readlink(entry->path,
                          target,
                          sizeof(target) - 1);

        if (length >= 0) {
            target[length] = '\0';
            printf(" -> %s", target);
        }
    }

    printf("\n");
}
