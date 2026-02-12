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
 * Sets ctx->log_fd. Returns 0 on success, -1 on error.
 */
int hub_log_open(hub_ctx *ctx);

/*
 * hub_log_close — Close the log fd.
 */
void hub_log_close(hub_ctx *ctx);

/*
 * hub_log_write — Append a timestamped entry to hub.log.
 *
 * Format: "YYYY-MM-DDTHH:MM:SS <message>\n"
 */
void hub_log_write(hub_ctx *ctx, const char *fmt, ...);

/*
 * hub_log_show — Print the last N entries from hub.log to stdout.
 *
 * Returns 0 on success, -1 on error.
 */
int hub_log_show(hub_ctx *ctx, int n);

/*
 * hub_chat_log — Send a message to hub.chat channel.
 *
 * Prefixes message with "HUB:" for machine-readability.
 */
void hub_chat_log(hub_ctx *ctx, const char *fmt, ...);

#endif /* NBS_HUB_LOG_H */
