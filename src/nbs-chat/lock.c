/*
 * lock.c — fcntl-based file locking
 */

#include "lock.h"
#include "chat_file.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int chat_lock_acquire(const char *chat_path) {
    ASSERT_MSG(chat_path != NULL, "chat_lock_acquire: path is NULL");

    /* Build lock file path: chat_path + ".lock" */
    size_t path_len = strlen(chat_path);
    ASSERT_MSG(path_len + 6 <= MAX_PATH_LEN,
               "chat_lock_acquire: path too long: %zu + 6 > %d", path_len, MAX_PATH_LEN);
    char lock_path[MAX_PATH_LEN];
    snprintf(lock_path, sizeof(lock_path), "%s.lock", chat_path);

    int fd = open(lock_path, O_RDWR | O_CREAT, 0600);
    if (fd < 0) {
        fprintf(stderr, "warning: chat_lock_acquire: open failed for %s: %s\n",
                lock_path, strerror(errno));
        return -1;
    }

    struct flock fl = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0, /* Lock entire file */
    };

    /* Block until lock is acquired */
    if (fcntl(fd, F_SETLKW, &fl) < 0) {
        fprintf(stderr, "warning: chat_lock_acquire: fcntl lock failed for %s: %s\n",
                lock_path, strerror(errno));
        close(fd);
        return -1;
    }

    ASSERT_MSG(fd >= 0, "chat_lock_acquire: internal error — fd %d after successful lock", fd);
    return fd;
}

void chat_lock_release(int lock_fd) {
    ASSERT_MSG(lock_fd >= 0, "chat_lock_release: invalid fd %d", lock_fd);

    struct flock fl = {
        .l_type = F_UNLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0,
    };

    fcntl(lock_fd, F_SETLK, &fl);
    close(lock_fd);
}
