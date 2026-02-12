/*
 * chat_file.c — Chat file protocol implementation
 */

#include "chat_file.h"
#include "base64.h"
#include "lock.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* --- Internal helpers --- */

static void get_timestamp(char *buf, size_t buf_size) {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    strftime(buf, buf_size, "%Y-%m-%dT%H:%M:%S%z", tm);
}

/*
 * Compute self-consistent file-length.
 * The file-length header line is: "file-length: N\n"
 * where N is the total file size INCLUDING the line containing N.
 * This is self-referential: we must solve for N.
 */
static long compute_file_length(const char *content_without_length) {
    /* Write content without the file-length line, measure it */
    size_t base_size = strlen(content_without_length); /* content already ends with \n */

    /* The line we will insert is "file-length: N\n" = 14 + digits(N) chars */
    /* Try with current digit count */
    char size_str[32];
    snprintf(size_str, sizeof(size_str), "%zu", base_size);
    size_t digits = strlen(size_str);

    long candidate = base_size + 14 + digits;

    /* Check if adding the line changed the digit count */
    snprintf(size_str, sizeof(size_str), "%ld", candidate);
    if (strlen(size_str) != digits) {
        candidate = base_size + 14 + strlen(size_str);
    }

    return candidate;
}

static int parse_participants(const char *line, participant_t *parts, int max_parts) {
    int count = 0;
    const char *p = line;

    while (*p && count < max_parts) {
        /* Skip whitespace and commas */
        while (*p == ' ' || *p == ',') p++;
        if (*p == '\0' || *p == '\n') break;

        /* Read handle */
        const char *start = p;
        while (*p && *p != '(' && *p != ',' && *p != '\n') p++;

        size_t handle_len = p - start;
        if (handle_len == 0 || handle_len >= MAX_HANDLE_LEN) break;

        strncpy(parts[count].handle, start, handle_len);
        parts[count].handle[handle_len] = '\0';

        /* Read count if present */
        parts[count].count = 0;
        if (*p == '(') {
            p++; /* skip ( */
            parts[count].count = atoi(p);
            while (*p && *p != ')') p++;
            if (*p == ')') p++;
        }

        count++;
    }

    return count;
}

static void format_participants(const participant_t *parts, int count,
                                 char *buf, size_t buf_size) {
    size_t offset = 0;
    for (int i = 0; i < count; i++) {
        int written;
        if (i > 0) {
            written = snprintf(buf + offset, buf_size - offset, ", ");
            if (written > 0) offset += written;
        }
        written = snprintf(buf + offset, buf_size - offset,
                           "%s(%d)", parts[i].handle, parts[i].count);
        if (written > 0) offset += written;
    }
}

static int update_participants(participant_t *parts, int count,
                                const char *handle, int max_parts) {
    /* Find existing participant */
    for (int i = 0; i < count; i++) {
        if (strcmp(parts[i].handle, handle) == 0) {
            parts[i].count++;
            return count;
        }
    }

    /* Add new participant */
    if (count >= max_parts) return count;
    strncpy(parts[count].handle, handle, MAX_HANDLE_LEN - 1);
    parts[count].handle[MAX_HANDLE_LEN - 1] = '\0';
    parts[count].count = 1;
    return count + 1;
}

/* --- Public API --- */

int chat_create(const char *path) {
    ASSERT_MSG(path != NULL, "chat_create: path is NULL");

    /* Check if file already exists */
    struct stat st;
    if (stat(path, &st) == 0) {
        return -1; /* Already exists */
    }

    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));

    /* Build content without file-length line */
    char content[1024];
    int len = snprintf(content, sizeof(content),
        "=== nbs-chat ===\n"
        "last-writer: system\n"
        "last-write: %s\n"
        "participants: \n"
        "---\n",
        timestamp);

    if (len < 0 || (size_t)len >= sizeof(content)) return -2;

    long file_len = compute_file_length(content);

    /* Now write the actual file with file-length inserted */
    FILE *f = fopen(path, "w");
    if (!f) return -2;

    fprintf(f, "=== nbs-chat ===\n");
    fprintf(f, "last-writer: system\n");
    fprintf(f, "last-write: %s\n", timestamp);
    fprintf(f, "file-length: %ld\n", file_len);
    fprintf(f, "participants: \n");
    fprintf(f, "---\n");
    fclose(f);

    /* Postcondition: verify file-length matches actual size */
    if (stat(path, &st) == 0) {
        ASSERT_MSG(st.st_size == file_len,
                   "chat_create postcondition: file-length header %ld != actual size %ld",
                   file_len, (long)st.st_size);
    }

    return 0;
}

int chat_read(const char *path, chat_state_t *state) {
    ASSERT_MSG(path != NULL, "chat_read: path is NULL");
    ASSERT_MSG(state != NULL, "chat_read: state is NULL");

    memset(state, 0, sizeof(*state));

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char line[MAX_MESSAGE_LEN];
    int in_header = 0;
    int past_header = 0;

    /* Temporary message storage */
    state->messages = malloc(sizeof(chat_message_t) * MAX_MESSAGES);
    if (!state->messages) {
        fclose(f);
        return -1;
    }
    state->message_count = 0;

    while (fgets(line, sizeof(line), f)) {
        /* Strip trailing newline */
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }

        if (strcmp(line, "=== nbs-chat ===") == 0) {
            in_header = 1;
            continue;
        }

        if (in_header && strcmp(line, "---") == 0) {
            in_header = 0;
            past_header = 1;
            continue;
        }

        if (in_header) {
            /* Parse header fields */
            if (strncmp(line, "last-writer: ", 13) == 0) {
                snprintf(state->last_writer, MAX_HANDLE_LEN, "%.*s",
                         (int)(MAX_HANDLE_LEN - 1), line + 13);
            } else if (strncmp(line, "last-write: ", 12) == 0) {
                snprintf(state->last_write, sizeof(state->last_write), "%.*s",
                         (int)(sizeof(state->last_write) - 1), line + 12);
            } else if (strncmp(line, "file-length: ", 13) == 0) {
                state->file_length = atol(line + 13);
            } else if (strncmp(line, "participants: ", 14) == 0) {
                state->participant_count = parse_participants(
                    line + 14, state->participants, MAX_PARTICIPANTS);
            }
            continue;
        }

        if (past_header && len > 0 && state->message_count < MAX_MESSAGES) {
            /* Decode base64 message */
            size_t decoded_max = base64_decoded_size(len);
            unsigned char *decoded = malloc(decoded_max + 1);
            if (!decoded) continue;

            int decoded_len = base64_decode(line, len, decoded, decoded_max);
            if (decoded_len < 0) {
                free(decoded);
                continue;
            }
            decoded[decoded_len] = '\0';

            /* Parse "handle: content" */
            char *colon = strstr((char *)decoded, ": ");
            if (colon) {
                size_t handle_len = colon - (char *)decoded;
                if (handle_len < MAX_HANDLE_LEN) {
                    chat_message_t *msg = &state->messages[state->message_count];
                    strncpy(msg->handle, (char *)decoded, handle_len);
                    msg->handle[handle_len] = '\0';
                    msg->content = strdup(colon + 2);
                    msg->content_len = decoded_len - handle_len - 2;
                    state->message_count++;
                }
            }

            free(decoded);
        }
    }

    /* Invariant: message_count must be within bounds */
    ASSERT_MSG(state->message_count >= 0 && state->message_count <= MAX_MESSAGES,
               "chat_read: message_count %d out of bounds [0, %d]",
               state->message_count, MAX_MESSAGES);

    fclose(f);
    return 0;
}

int chat_send(const char *path, const char *handle, const char *message) {
    ASSERT_MSG(path != NULL, "chat_send: path is NULL");
    ASSERT_MSG(handle != NULL, "chat_send: handle is NULL");
    ASSERT_MSG(message != NULL, "chat_send: message is NULL");

    int lock_fd = chat_lock_acquire(path);
    if (lock_fd < 0) return -1;

    /* Read current state */
    chat_state_t state;
    if (chat_read(path, &state) < 0) {
        chat_lock_release(lock_fd);
        return -1;
    }

    /* Build the message line: "handle: message" */
    size_t raw_len = strlen(handle) + 2 + strlen(message); /* "handle: msg" */
    char *raw = malloc(raw_len + 1);
    if (!raw) {
        chat_state_free(&state);
        chat_lock_release(lock_fd);
        return -1;
    }
    snprintf(raw, raw_len + 1, "%s: %s", handle, message);

    /* Postcondition: raw message was fully written */
    ASSERT_MSG(raw_len > 0,
               "chat_send: raw message length is zero for handle '%s'", handle);

    /* Base64 encode */
    size_t encoded_size = base64_encoded_size(raw_len);
    char *encoded = malloc(encoded_size);
    if (!encoded) {
        free(raw);
        chat_state_free(&state);
        chat_lock_release(lock_fd);
        return -1;
    }
    base64_encode((unsigned char *)raw, raw_len, encoded, encoded_size);
    free(raw);

    /* Update participants */
    state.participant_count = update_participants(
        state.participants, state.participant_count, handle, MAX_PARTICIPANTS);

    /* Update header fields */
    snprintf(state.last_writer, MAX_HANDLE_LEN, "%s", handle);
    char timestamp[64];
    get_timestamp(timestamp, sizeof(timestamp));
    snprintf(state.last_write, sizeof(state.last_write), "%s", timestamp);

    /* Build the file content without file-length line */
    /* First, calculate total size needed */
    char parts_str[4096];
    format_participants(state.participants, state.participant_count,
                        parts_str, sizeof(parts_str));

    /* Build header */
    char header[8192];
    int header_len = snprintf(header, sizeof(header),
        "=== nbs-chat ===\n"
        "last-writer: %s\n"
        "last-write: %s\n"
        "participants: %s\n"
        "---\n",
        state.last_writer, state.last_write, parts_str);

    /* Calculate total content size (header + existing messages + new message) */

    /* Read the raw file to get existing encoded lines */
    FILE *f = fopen(path, "r");
    if (!f) {
        free(encoded);
        chat_state_free(&state);
        chat_lock_release(lock_fd);
        return -1;
    }

    /* Collect existing encoded lines */
    char **encoded_lines = NULL;
    int encoded_line_count = 0;
    char line_buf[MAX_MESSAGE_LEN];
    int past_delim = 0;

    while (fgets(line_buf, sizeof(line_buf), f)) {
        size_t ll = strlen(line_buf);
        while (ll > 0 && (line_buf[ll-1] == '\n' || line_buf[ll-1] == '\r'))
            line_buf[--ll] = '\0';

        if (!past_delim) {
            if (strcmp(line_buf, "---") == 0 && encoded_line_count == 0) {
                /* This is the closing delimiter of the header */
                /* But we need to distinguish from "=== nbs-chat ===" */
                /* Check if we've seen the opening marker */
                past_delim = 1;
            }
            continue;
        }

        if (ll > 0) {
            encoded_lines = realloc(encoded_lines,
                                     sizeof(char *) * (encoded_line_count + 1));
            encoded_lines[encoded_line_count] = strdup(line_buf);
            encoded_line_count++;
        }
    }
    fclose(f);

    /* Invariant: encoded_line_count must be non-negative */
    ASSERT_MSG(encoded_line_count >= 0,
               "chat_send: encoded_line_count went negative: %d", encoded_line_count);

    /* Calculate content without file-length for size computation */
    size_t content_size = header_len;
    for (int i = 0; i < encoded_line_count; i++) {
        content_size += strlen(encoded_lines[i]) + 1; /* +1 for \n */
    }
    content_size += strlen(encoded) + 1; /* new message + \n */

    char *content_no_fl = malloc(content_size + 1);
    if (!content_no_fl) {
        for (int i = 0; i < encoded_line_count; i++) free(encoded_lines[i]);
        free(encoded_lines);
        free(encoded);
        chat_state_free(&state);
        chat_lock_release(lock_fd);
        return -1;
    }

    size_t offset = 0;
    memcpy(content_no_fl + offset, header, header_len);
    offset += header_len;
    for (int i = 0; i < encoded_line_count; i++) {
        size_t ll = strlen(encoded_lines[i]);
        memcpy(content_no_fl + offset, encoded_lines[i], ll);
        offset += ll;
        content_no_fl[offset++] = '\n';
    }
    size_t enc_len = strlen(encoded);
    memcpy(content_no_fl + offset, encoded, enc_len);
    offset += enc_len;
    content_no_fl[offset++] = '\n';
    content_no_fl[offset] = '\0';

    long file_len = compute_file_length(content_no_fl);

    /* Write the file with file-length inserted after last-write line */
    f = fopen(path, "w");
    if (!f) {
        free(content_no_fl);
        for (int i = 0; i < encoded_line_count; i++) free(encoded_lines[i]);
        free(encoded_lines);
        free(encoded);
        chat_state_free(&state);
        chat_lock_release(lock_fd);
        return -1;
    }

    fprintf(f, "=== nbs-chat ===\n");
    fprintf(f, "last-writer: %s\n", state.last_writer);
    fprintf(f, "last-write: %s\n", state.last_write);
    fprintf(f, "file-length: %ld\n", file_len);
    fprintf(f, "participants: %s\n", parts_str);
    fprintf(f, "---\n");
    for (int i = 0; i < encoded_line_count; i++) {
        fprintf(f, "%s\n", encoded_lines[i]);
    }
    fprintf(f, "%s\n", encoded);
    fclose(f);

    /* Postcondition: verify file-length matches actual size */
    struct stat st;
    if (stat(path, &st) == 0) {
        ASSERT_MSG(st.st_size == file_len,
                   "chat_send postcondition: file-length header %ld != actual size %ld",
                   file_len, (long)st.st_size);
    }

    /* Cleanup */
    free(content_no_fl);
    for (int i = 0; i < encoded_line_count; i++) free(encoded_lines[i]);
    free(encoded_lines);
    free(encoded);
    chat_state_free(&state);
    chat_lock_release(lock_fd);

    return 0;
}

int chat_poll(const char *path, const char *handle, int timeout_secs) {
    ASSERT_MSG(path != NULL, "chat_poll: path is NULL");
    ASSERT_MSG(handle != NULL, "chat_poll: handle is NULL");
    ASSERT_MSG(timeout_secs >= 0,
               "chat_poll: timeout_secs is negative: %d", timeout_secs);

    /* Get initial message count */
    chat_state_t state;
    if (chat_read(path, &state) < 0) return -1;
    int initial_count = state.message_count;
    chat_state_free(&state);

    for (int elapsed = 0; elapsed < timeout_secs; elapsed++) {
        sleep(1);

        if (chat_read(path, &state) < 0) return -1;

        if (state.message_count > initial_count) {
            /* Check if any new message is from someone other than handle */
            for (int i = initial_count; i < state.message_count; i++) {
                if (strcmp(state.messages[i].handle, handle) != 0) {
                    chat_state_free(&state);
                    return 0; /* New message from other participant */
                }
            }
        }

        chat_state_free(&state);
    }

    return 3; /* Timeout */
}

void chat_state_free(chat_state_t *state) {
    if (!state) return;
    if (state->messages) {
        for (int i = 0; i < state->message_count; i++) {
            free(state->messages[i].content);
        }
        free(state->messages);
        state->messages = NULL;
    }
    state->message_count = 0;
}
