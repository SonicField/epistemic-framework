/*
 * lock.c — fcntl-based file locking
 */

#include "lock.h"
#include "chat_file.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int chat_lock_acquire(const char *chat_path) {
    ASSERT_MSG(chat_path != NULL, "chat_lock_acquire: path is NULL");

    /* Build lock file path: chat_path + ".lock" */
    size_t path_len = strlen(chat_path);
    char lock_path[path_len + 6]; /* +5 for ".lock" +1 for null */
    snprintf(lock_path, sizeof(lock_path), "%s.lock", chat_path);

    int fd = open(lock_path, O_RDWR | O_CREAT, 0644);
    if (fd < 0) return -1;

    struct flock fl = {
        .l_type = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0, /* Lock entire file */
    };

    /* Block until lock is acquired */
    if (fcntl(fd, F_SETLKW, &fl) < 0) {
        close(fd);
        return -1;
    }

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
