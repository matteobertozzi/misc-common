/*
 *   Copyright 2012 Matteo Bertozzi
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#define FUSE_USE_VERSION 29
#define _POSIX_C_SOURCE  200809L
#define _XOPEN_SOURCE 700

#include <fuse.h>

#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#ifdef HAVE_SETXATTR
    #include <sys/xattr.h>
#endif

#include "crypto.h"
#include "block.h"
#include "util.h"

/* ============================================================================
 *  File-System helper struct
 */
struct aesfs {
    crypto_aes_t *aes;
    iocodec_t     codec;
    const char *  root;
    unsigned int  root_length;
};

static struct aesfs __aesfs;

static int aesfs_open (void) {
    /* Initialize AES */
    if ((__aesfs.aes = crypto_aes_from_input()) == NULL)
        return(-1);

    /* Init AES codec */
    __aesfs.codec.plug = &ioblock_aes_codec;
    __aesfs.codec.data.ptr = __aesfs.aes;

    return(0);
}

static void aesfs_close (void) {
    crypto_aes_close(__aesfs.aes);
}

/* ============================================================================
 *  AESFS File helpers
 */
struct aesfs_file {
    struct iofhead head;
    int fd;
};

static struct aesfs_file *aesfs_file_from_fd (int fd) {
    struct aesfs_file *file;

    if (!(file = (struct aesfs_file *) malloc(sizeof(struct aesfs_file)))) {
        close(fd);
        return(NULL);
    }

    file->fd = fd;
    if (iofhead_read(fd, &(file->head))) {
        file->head.magic = IOFHEAD_MAGIC;
        file->head.flags = 0;
        file->head.length = 0;
    }

    return(file);
}

static struct aesfs_file *aesfs_file_create (const char *path,
                                             int flags,
                                             mode_t mode)
{
    int fd;

    if ((fd = open(path, flags, mode)) < 0)
        return(NULL);

    return(aesfs_file_from_fd(fd));
}

static struct aesfs_file *aesfs_file_open (const char *path, int flags) {
    int fd;

    if ((fd = open(path, flags)) < 0)
        return(NULL);

    return(aesfs_file_from_fd(fd));
}

static int aesfs_file_sync (struct aesfs_file *file) {
    return(iofhead_write(file->fd, &(file->head)));
}

static void aesfs_file_close (struct aesfs_file *file) {
    close(file->fd);
    free(file);
}

/* ============================================================================
 *  AESFS Path helpers
 */
#define __aes_align(x)              ((((x) + 15) & -16) + 16)
#define __hex_byte(b)               (isdigit(b) ? (b - '0') : (b - 'a' + 10))
#define __two_hex_bytes(b0, b1)     ((__hex_byte(b0) << 4) + __hex_byte(b1))

static char *__file_name_encode (char *path, const char *part, size_t size) {
    const unsigned char *p;
    unsigned int bufsize;
    char buffer[1024];

    if (crypto_aes_encrypt(__aesfs.aes, part, size, buffer, &bufsize))
        return(NULL);

    for (p = (const unsigned char *)buffer; bufsize-- > 0; ++p)
        path += snprintf(path, 4, "%02x", *p);

    return(path);
}

static char *__file_name_decode (char *path, const char *part, size_t size) {
    unsigned char buffer[1024];
    unsigned int part_size;
    unsigned char *pbuf;

    /* Check if name is encoded */
    if ((size & 15) != 0) {
        memcpy(path, part, size);
        return(path + size);
    }

    pbuf = buffer;
    while (size > 0) {
        *pbuf++ = __two_hex_bytes(part[0], part[1]);
        part += 2;
        size -= 2;
    }

    if (crypto_aes_decrypt(__aesfs.aes, buffer, pbuf - buffer, path, &part_size))
        return(NULL);

    return(path + part_size);
}

static int __file_path_transform (char *realpath,
                                  const char *path,
                                  char *(*func) (char *, const char *, size_t))
{
    size_t psize;
    char *rp;
    char *p;

    rp = realpath + __aesfs.root_length;
    memcpy(realpath, __aesfs.root, __aesfs.root_length);
    while ((p = strchr(path, '/')) != NULL) {
        if ((psize = (p - path)) > 0) {
            if ((rp = func(rp, path, psize)) == NULL)
                return(-1);
        }

        path = p + 1;
        *rp++ = '/';
    }

    if (*path != '\0') {
        if ((rp = func(rp, path, strlen(path))) == NULL)
            return(-2);
    }

    *rp = '\0';
    return(0);
}

static char *aesfs_file_path_encode (const char *path) {
    const char *ppath;
    char *realpath;
    size_t psize;
    char *p;

    ppath = path;
    psize = __aesfs.root_length;
    for (; (p = strchr(ppath, '/')) != NULL; ppath = p + 1)
        psize += (__aes_align(p - ppath) << 1) + 1;
    psize += __aes_align(strlen(ppath)) << 1;

    if ((realpath = (char *) malloc(1 + psize)) == NULL)
        return(NULL);

    if (__file_path_transform(realpath, path, __file_name_encode)) {
        free(realpath);
        return(NULL);
    }

    return(realpath);
}

static int aesfs_file_name_decode (char *dst, const char *name) {
    char *p;
    if ((p = __file_name_decode(dst, name, strlen(name))) == NULL)
        return(-1);
    *p = '\0';
    return(0);
}

static int aesfs_file_stat (const char *path, struct stat *stbuf) {
    int res;

    if ((res = lstat(path, stbuf)) >= 0) {
        int fd;

        if ((fd = open(path, O_RDONLY)) > 0) {
            struct iofhead fhead;
            if (!iofhead_read(fd, &fhead))
                stbuf->st_size = fhead.length;
            close(fd);
        }
    }

    return(res);
}

/* ============================================================================
 *  FUSE Helpers
 */
#define __fuse_sys_bypass(func, path, ...)                                  \
    do {                                                                    \
        char *realpath;                                                     \
        int res;                                                            \
                                                                            \
        if ((realpath = aesfs_file_path_encode(path)) == NULL)              \
            return(-ENOMEM);                                                \
                                                                            \
        res = func(realpath, ##__VA_ARGS__);                                \
                                                                            \
        free(realpath);                                                     \
                                                                            \
        return((res < 0) ? -errno : 0);                                     \
    } while (0);

/* ============================================================================
 *  FUSE operations
 */
static int __getattr (const char *path, struct stat *stbuf) {
    __fuse_sys_bypass(aesfs_file_stat, path, stbuf);
}

static int __readlink (const char *path, char *buf, size_t size) {
    char *realpath;
    int res;

    if ((realpath = aesfs_file_path_encode(path)) == NULL)
        return(-ENOMEM);

    if ((res = readlink(realpath, buf, size - 1)) > 0) {
        buf[res] = '\0';
        free(realpath);
        return(0);
    }

    free(realpath);
    return(-errno);
}

static int __readdir (const char *path,
                      void *buf,
                      fuse_fill_dir_t filler,
                      off_t offset,
                      struct fuse_file_info *fi)
{
    char realname[1024];
    struct dirent *de;
    struct stat st;
    char *realpath;
    DIR *dp;

    (void)offset;
    (void)fi;

    if ((realpath = aesfs_file_path_encode(path)) == NULL)
        return(-ENOMEM);

    if ((dp = opendir(realpath)) == NULL) {
        free(realpath);
        return(-errno);
    }

    free(realpath);

    while ((de = readdir(dp)) != NULL) {
        if (aesfs_file_name_decode(realname, de->d_name))
            continue;

        memset(&st, 0, sizeof(struct stat));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, realname, &st, 0))
            break;
    }

    closedir(dp);
    return(0);
}

static int __mknod (const char *path, mode_t mode, dev_t rdev) {
    char *realpath;
    int res;

    if ((realpath = aesfs_file_path_encode(path)) == NULL)
        return(-ENOMEM);

    /* On Linux this could just be 'mknod(path, mode, rdev)'
     * but this is more portable.
     */
    if (S_ISREG(mode)) {
        if ((res = open(realpath, O_CREAT | O_EXCL | O_WRONLY, mode)) >= 0) {
            struct iofhead head;
            memset(&head, 0, sizeof(struct iofhead));
            iofhead_write(res, &head);
            res = close(res);
        }
    } else if (S_ISFIFO(mode)) {
        res = mkfifo(realpath, mode);
    } else {
        res = mknod(realpath, mode, rdev);
    }

    free(realpath);
    return((res < 0) ? -errno : 0);
}

static int __mkdir (const char *path, mode_t mode) {
    __fuse_sys_bypass(mkdir, path, mode);
}

static int __unlink (const char *path) {
    __fuse_sys_bypass(unlink, path);
}

static int __rmdir (const char *path) {
    __fuse_sys_bypass(rmdir, path);
}

static int __symlink (const char *from, const char *to) {
    __fuse_sys_bypass(symlink, from, to);
}

static int __rename (const char *from, const char *to) {
    char *realfrom;
    char *realto;
    int res;

    if ((realfrom = aesfs_file_path_encode(from)) == NULL)
        return(-ENOMEM);

    if ((realto = aesfs_file_path_encode(to)) == NULL) {
        free(realfrom);
        return(-ENOMEM);
    }

    res = rename(realfrom, realto);

    free(realfrom);
    free(realto);
    return((res < 0) ? -errno : 0);
}

static int __link (const char *from, const char *to) {
    __fuse_sys_bypass(link, from, to);
}

static int __chmod (const char *path, mode_t mode) {
    __fuse_sys_bypass(chmod, path, mode);
}

static int __chown (const char *path, uid_t uid, gid_t gid) {
    __fuse_sys_bypass(lchown, path, uid, gid);
}

static int __truncate (const char *path, off_t size) {
    __fuse_sys_bypass(truncate, path, size);
}

static int __utimens (const char *path, const struct timespec ts[2]) {
    struct timeval tv[2];

    tv[0].tv_sec  = ts[0].tv_sec;
    tv[0].tv_usec = ts[0].tv_nsec / 1000;
    tv[1].tv_sec  = ts[1].tv_sec;
    tv[1].tv_usec = ts[1].tv_nsec / 1000;

    __fuse_sys_bypass(utimes, path, tv);
}

static int __create (const char *path, mode_t mode, struct fuse_file_info *fi) {
    struct aesfs_file *file;
    char *realpath;

    if ((realpath = aesfs_file_path_encode(path)) == NULL)
        return(-ENOMEM);

    fi->flags &= ~O_WRONLY;
    fi->flags |= O_RDWR;

    if ((file = aesfs_file_create(realpath, fi->flags, mode)) == NULL) {
        free(realpath);
        return(-errno);
    }

    fi->fh = (uint64_t)file;
    free(realpath);
    return(0);
}

static int __open (const char *path, struct fuse_file_info *fi) {
    struct aesfs_file *file;
    char *realpath;

    fi->flags &= ~O_RDONLY;
    fi->flags &= ~O_WRONLY;
    fi->flags |= O_RDWR;

    if ((realpath = aesfs_file_path_encode(path)) == NULL)
        return(-ENOMEM);

    if ((file = aesfs_file_open(realpath, fi->flags)) == NULL) {
        free(realpath);
        return(-errno);
    }

    fi->fh = (uint64_t)file;
    free(realpath);
    return(0);
}

static int __read (const char *path,
                   char *buf,
                   size_t size,
                   off_t offset,
                   struct fuse_file_info *fi)
{
    struct aesfs_file *file = (struct aesfs_file *)fi->fh;
    int rd = ioblock_read(&__aesfs.codec, file->fd, buf, size, offset);
    return((rd < 0) ? -EIO : rd);
}

static int __write (const char *path,
                    const char *buf,
                    size_t size,
                    off_t offset,
                    struct fuse_file_info *fi)
{
    struct aesfs_file *file = (struct aesfs_file *)fi->fh;
    int wr;

    if ((wr = ioblock_write(&__aesfs.codec, file->fd, buf, size, offset)) > 0) {
        if ((offset + wr) > file->head.length) {
            file->head.length = offset + wr;
            aesfs_file_sync(file);
        }
    }

    return((wr < 0) ? -EIO : wr);
}

static int __statfs (const char *path, struct statvfs *stbuf) {
    __fuse_sys_bypass(statvfs, path, stbuf);
}

static int __release (const char *path, struct fuse_file_info *fi) {
    struct aesfs_file *file = (struct aesfs_file *)fi->fh;
    aesfs_file_close(file);
    return(0);
}

static int __fsync (const char *path,
                    int isdatasync,
                    struct fuse_file_info *fi)
{
    struct aesfs_file *file = (struct aesfs_file *)fi->fh;
    fsync(file->fd);
    return(0);
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int __setxattr (const char *path,
                       const char *name,
                       const char *value,
                       size_t size,
                       int flags)
{
    __fuse_sys_bypass(lsetxattr, path, name, value, size, flags);
}

static int __getxattr (const char *path,
                       const char *name,
                       char *value,
                       size_t size)
{
    __fuse_sys_bypass(lgetxattr, path, name, value, size);
}

static int __listxattr (const char *path, char *list, size_t size) {
    __fuse_sys_bypass(llistxattr, path, list, size);
}

static int __removexattr (const char *path, const char *name) {
    __fuse_sys_bypass(lremovexattr, path, name);
}
#endif /* HAVE_SETXATTR */

/* ============================================================================
 *  FUSE operations table
 */
static struct fuse_operations __aesfs_fuse = {
    .getattr    = __getattr,
    .readlink   = __readlink,
    .readdir    = __readdir,
    .mknod      = __mknod,
    .mkdir      = __mkdir,
    .symlink    = __symlink,
    .unlink     = __unlink,
    .rmdir      = __rmdir,
    .rename     = __rename,
    .link       = __link,
    .chmod      = __chmod,
    .chown      = __chown,
    .truncate   = __truncate,
    .utimens    = __utimens,
    .open       = __open,
    .create     = __create,
    .read       = __read,
    .write      = __write,
    .statfs     = __statfs,
    .release    = __release,
    .fsync      = __fsync,
#ifdef HAVE_SETXATTR
    .setxattr    = __setxattr,
    .getxattr    = __getxattr,
    .listxattr   = __listxattr,
    .removexattr = __removexattr,
#endif
};

/* ============================================================================
 *  AESFS-FUSE Main
 */
enum {
    AESFS_KEY_HELP,
    AESFS_KEY_VERSION,
};

#define AESFS_OPT(t, p, v)         { t, __builtin_offsetof(struct aesfs, p), v }

static struct fuse_opt __aesfs_opts[] = {
    AESFS_OPT("root=%s", root, 0),

    FUSE_OPT_KEY("-V", AESFS_KEY_VERSION),
    FUSE_OPT_KEY("--version", AESFS_KEY_VERSION),
    FUSE_OPT_KEY("-h", AESFS_KEY_HELP),
    FUSE_OPT_KEY("--help", AESFS_KEY_HELP),
    FUSE_OPT_END
};

static int __aesfs_opt_proc (void *data,
                             const char *arg,
                             int key,
                             struct fuse_args *outargs)
{
    switch (key) {
        case AESFS_KEY_HELP:
            fprintf(stderr,
                    "usage: %s mountpoint --root rootpath [options]\n"
                    "\n"
                    "general options:\n"
                    "    -o opt,[opt...]    mount options\n"
                    "    -h   --help        print help\n"
                    "    -V   --version     print version\n"
                    "\n"
                    "AESFS options:\n"
                    "    -o root=ROOT-PATH  root path\n", outargs->argv[0]);
            fuse_opt_add_arg(outargs, "-ho");
            fuse_main(outargs->argc, outargs->argv, &__aesfs_fuse, NULL);
            exit(EXIT_FAILURE);

        case AESFS_KEY_VERSION:
            fprintf(stderr, "AESFS version 0.1\n");
            fuse_opt_add_arg(outargs, "--version");
            fuse_main(outargs->argc, outargs->argv, &__aesfs_fuse, NULL);
            exit(EXIT_SUCCESS);
    }
    return(1);
}

int main (int argc, char **argv) {
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    int res;

    __aesfs.root = NULL;
    __aesfs.root_length = 0;

    fuse_opt_parse(&args, &__aesfs, __aesfs_opts, __aesfs_opt_proc);
    __aesfs.root_length = (__aesfs.root != NULL) ? strlen(__aesfs.root) : 0;
    if (!__aesfs.root_length) {
        fprintf(stderr, "aesfs: root path parameter (add -o root=path)\n");
        return(EXIT_FAILURE);
    }

    if (__aesfs.root[__aesfs.root_length - 1] == '/')
        __aesfs.root_length--;

    if ((aesfs_open()) < 0)
        return(EXIT_FAILURE);

    res = fuse_main(args.argc, args.argv, &__aesfs_fuse, NULL);

    aesfs_close();
    return(res);
}

