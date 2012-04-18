/**
 * Copy to files without polluting the page cache.
 */
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

static ssize_t __fdcopy (int sfd, int dfd, size_t length) {
    ssize_t copied = 0;
    ssize_t rd, wr;
    int pfd[2];

    if (pipe(pfd) < 0) {
        perror("pipe()");
        return(-1);
    }

    while (copied < length) {
        rd = splice(sfd, NULL, pfd[1], NULL,
                    length - copied,
                    SPLICE_F_MORE | SPLICE_F_MOVE);
        if (!rd)
            break;

        if (rd < 0) {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            perror("splice from file");
            close(pfd[0]);
            close(pfd[1]);
            return(-copied);
        }

        posix_fadvise(sfd, 0, copied, POSIX_FADV_DONTNEED);
        while (rd > 0) {
            wr = splice(pfd[0], NULL, dfd, NULL, rd, SPLICE_F_MORE | SPLICE_F_MOVE);
            if (wr <= 0) {
                if (errno == EINTR || errno == EAGAIN)
                    continue;
                perror("splice() to destination");
                close(pfd[0]);
                close(pfd[1]);
                return(-copied);
            }

            posix_fadvise(dfd, 0, 0, POSIX_FADV_DONTNEED);
            copied += wr;
            rd -= wr;
        }
    }

    close(pfd[0]);
    close(pfd[1]);
    return(copied);
}

static ssize_t __copy (const char *srcpath, const char *dstpath) {
    struct stat stbuf;
    ssize_t result;
    int sfd, dfd;

    if ((sfd = open(srcpath, O_RDONLY)) < 0) {
        perror("open() source file");
        return(-1);
    }

    if (fstat(sfd, &stbuf) < 0) {
        perror("fstat()");
        close(sfd);
        return(-1);
    }

    if ((dfd = open(dstpath, O_WRONLY | O_CREAT | O_TRUNC)) < 0) {
        perror("open() destination file");
        close(sfd);
        return(-1);
    }

    result = __fdcopy(sfd, dfd, stbuf.st_size);

    close(dfd);
    close(sfd);

    return(result);
}

int main (int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: srv-cp <source> <destination>\n");
        return(1);
    }

    return(!!__copy(argv[1], argv[2]));
}
