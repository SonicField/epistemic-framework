/*
 * lock.h — File locking for nbs-chat
 *
 * Uses fcntl F_SETLKW for POSIX advisory locking on companion .lock files.
 * Lock is held for the duration of a read-modify-write cycle.
 */

#ifndef NBS_LOCK_H
#define NBS_LOCK_H

/*
 * chat_lock_acquire — Acquire exclusive lock on chat file.
 *
 * Opens/creates the companion .lock file and acquires an exclusive lock.
 * Blocks until the lock is available.
 *
 * Preconditions:
 *   - chat_path != NULL
 *   - chat_path is a valid path (companion .lock file will be created alongside)
 *
 * Returns:
 *   - File descriptor for the lock file (>= 0) on success
 *   - -1 on error
 *
 * The caller MUST call chat_lock_release with the returned fd when done.
 */
int chat_lock_acquire(const char *chat_path);

/*
 * chat_lock_release — Release exclusive lock.
 *
 * Preconditions:
 *   - lock_fd is a valid fd returned by chat_lock_acquire
 *
 * Closes the lock file descriptor, releasing the lock.
 */
void chat_lock_release(int lock_fd);

#endif /* NBS_LOCK_H */
