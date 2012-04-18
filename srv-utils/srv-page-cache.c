/**
 * Add and Remove file from page cache.
 * Useful to warm-up things.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

static void __print_usage (void) {
    fprintf(stderr, "Usage: srv-page-cache <opts> path [path ...]\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "    add      Add file to page cache\n");
    fprintf(stderr, "    rm       Remove file from page cache\n");
}

int main (int argc, char **argv) {
    int advise;
    int fd;
    int i;

    if (argc < 3) {
        __print_usage();
        return(1);
    }

    if (!strcmp(argv[1], "rm")) {
        advise = POSIX_FADV_DONTNEED;
    } else if (!strcmp(argv[1], "add")) {
        advise = POSIX_FADV_WILLNEED;
    } else {
        __print_usage();
        return(1);
    }

    for (i = 2; i < argc; i++) {
        if ((fd = open(argv[i], O_RDONLY)) >= 0) {
            posix_fadvise(fd, 0, 0, advise);
            close(fd);
        }
    }

    return(0);
}
