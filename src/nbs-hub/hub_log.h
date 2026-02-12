/*
 * hub_log.h — Append-only hub activity log
 *
 * All hub actions are logged to hub.log with timestamps.
 * The log is opened with O_APPEND for atomicity.
 */

#ifndef NBS_HUB_LOG_H
#define NBS_HUB_LOG_H

#include "hub_state.h"

/*
 * hub_log_open — Open hub.log for appending.
 *
 * Preconditions:
 *   - ctx != NULL
 *   - ctx->hub_dir contains a valid, null-terminated path
 *
 * Postconditions (on success):
 *   - ctx->log_fd >= 0 (open file descriptor)
 *   - Returns 0
 *
 * Postconditions (on failure):
 *   - ctx->log_fd is unchanged
 *   - Returns -1
 *   - Diagnostic printed to stderr
 */
int hub_log_open(hub_ctx *ctx);

/*
 * hub_log_close — Close the log fd.
 *
 * Preconditions:
 *   - ctx != NULL
 *
 * Postconditions:
 *   - ctx->log_fd == -1
 *   - If ctx->log_fd was >= 0 on entry, the fd is closed
 */
void hub_log_close(hub_ctx *ctx);

/*
 * hub_log_write — Append a timestamped entry to hub.log.
 *
 * Format: "YYYY-MM-DDTHH:MM:SS <message>\n"
 *
 * Preconditions:
 *   - ctx != NULL
 *   - fmt != NULL
 *   - ctx->log_fd >= 0 (log must be open)
 *
 * Postconditions:
 *   - On success, the formatted entry is appended to hub.log
 *   - On write failure or partial write, a warning is printed to stderr
 */
void hub_log_write(hub_ctx *ctx, const char *fmt, ...);

/*
 * hub_log_show — Print the last N entries from hub.log to stdout.
 *
 * Preconditions:
 *   - ctx != NULL
 *   - n >= 0
 *
 * Postconditions (on success):
 *   - Up to n lines printed to stdout
 *   - Returns 0
 *
 * Postconditions (on failure):
 *   - Diagnostic printed to stderr
 *   - Returns -1
 */
int hub_log_show(hub_ctx *ctx, int n);

/*
 * hub_chat_log — Send a message to hub.chat channel.
 *
 * Prefixes message with "HUB:" for machine-readability.
 *
 * Preconditions:
 *   - ctx != NULL
 *   - fmt != NULL
 *   - ctx->chat_path contains a valid path
 *
 * Postconditions:
 *   - If the chat file exists, the prefixed message is appended
 *   - If the chat file does not exist, no action is taken
 */
void hub_chat_log(hub_ctx *ctx, const char *fmt, ...);

#endif /* NBS_HUB_LOG_H */
