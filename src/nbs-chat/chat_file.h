/*
 * chat_file.h — Chat file protocol for nbs-chat
 *
 * File format:
 *   === nbs-chat ===
 *   last-writer: <handle>
 *   last-write: <ISO 8601 timestamp>
 *   file-length: <byte count>
 *   participants: <handle1>(N1), <handle2>(N2), ...
 *   ---
 *   <base64 encoded message 1>
 *   <base64 encoded message 2>
 *   ...
 *
 * Each message decodes to: "handle: message text"
 */

#ifndef NBS_CHAT_FILE_H
#define NBS_CHAT_FILE_H

#include <stddef.h>

/* Maximum sizes */
#define MAX_HANDLE_LEN 64
#define MAX_MESSAGE_LEN (1024 * 1024)  /* 1 MB per message */
#define MAX_MESSAGES 10000
#define MAX_PARTICIPANTS 256
#define MAX_PATH_LEN 4096

/* Decoded message */
typedef struct {
    char handle[MAX_HANDLE_LEN];
    char *content;     /* Dynamically allocated */
    size_t content_len;
} chat_message_t;

/* Participant info */
typedef struct {
    char handle[MAX_HANDLE_LEN];
    int count;
} participant_t;

/* Chat file state */
typedef struct {
    char last_writer[MAX_HANDLE_LEN];
    char last_write[64];  /* ISO 8601 timestamp */
    long file_length;
    participant_t participants[MAX_PARTICIPANTS];
    int participant_count;
    chat_message_t *messages;
    int message_count;
} chat_state_t;

/*
 * chat_create — Create a new empty chat file.
 *
 * Returns 0 on success, -1 if file already exists, -2 on I/O error.
 */
int chat_create(const char *path);

/*
 * chat_read — Read and parse a chat file.
 *
 * Caller must call chat_state_free() on the result.
 * Returns 0 on success, -1 on error.
 */
int chat_read(const char *path, chat_state_t *state);

/*
 * chat_send — Append a message to a chat file.
 *
 * Acquires lock, reads file, appends message, updates headers, writes back.
 * Returns 0 on success, -1 on error.
 */
int chat_send(const char *path, const char *handle, const char *message);

/*
 * chat_poll — Wait for a new message not from the given handle.
 *
 * Returns 0 if a new message arrived, 3 on timeout.
 */
int chat_poll(const char *path, const char *handle, int timeout_secs);

/*
 * chat_state_free — Release memory held by chat_state_t.
 */
void chat_state_free(chat_state_t *state);

#endif /* NBS_CHAT_FILE_H */
