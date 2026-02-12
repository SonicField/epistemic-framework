/*
 * hub_log.c — Append-only hub activity log
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#include "hub_log.h"
#include "chat_file.h"  /* chat_send, ASSERT_MSG */

int hub_log_open(hub_ctx *ctx)
{
    char path[HUB_MAX_PATH];
    int n = snprintf(path, sizeof(path), "%s/hub.log", ctx->hub_dir);
    ASSERT_MSG(n > 0 && n < (int)sizeof(path), "path overflow");

    ctx->log_fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (ctx->log_fd < 0) {
        fprintf(stderr, "error: cannot open hub.log: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

void hub_log_close(hub_ctx *ctx)
{
    if (ctx->log_fd >= 0) {
        close(ctx->log_fd);
        ctx->log_fd = -1;
    }
}

void hub_log_write(hub_ctx *ctx, const char *fmt, ...)
{
    ASSERT_MSG(ctx->log_fd >= 0, "hub_log_write: log not open");

    char msg[HUB_MAX_LINE];
    va_list ap;
    va_start(ap, fmt);
    int mlen = vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    ASSERT_MSG(mlen >= 0, "vsnprintf failed");

    /* Timestamp */
    time_t now = time(NULL);
    const char *ts = format_time(now);

    char entry[HUB_MAX_LINE];
    int elen = snprintf(entry, sizeof(entry), "%s %s\n", ts, msg);
    ASSERT_MSG(elen > 0 && elen < (int)sizeof(entry), "log entry overflow");

    /* O_APPEND makes this atomic for reasonable line lengths */
    ssize_t written = write(ctx->log_fd, entry, (size_t)elen);
    if (written < 0) {
        fprintf(stderr, "warning: hub.log write failed: %s\n", strerror(errno));
    }
}

int hub_log_show(hub_ctx *ctx, int n)
{
    char path[HUB_MAX_PATH];
    int pn = snprintf(path, sizeof(path), "%s/hub.log", ctx->hub_dir);
    ASSERT_MSG(pn > 0 && pn < (int)sizeof(path), "path overflow");

    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "error: cannot open hub.log: %s\n", strerror(errno));
        return -1;
    }

    /* Read all lines into a buffer, then print the last N */
    char *lines[10000];
    int count = 0;
    char line[HUB_MAX_LINE];

    while (fgets(line, sizeof(line), fp) && count < 10000) {
        lines[count] = strdup(line);
        ASSERT_MSG(lines[count] != NULL, "strdup failed");
        count++;
    }
    fclose(fp);

    int start = count - n;
    if (start < 0) start = 0;

    printf("=== Hub Log (last %d of %d) ===\n", count - start, count);
    for (int i = start; i < count; i++) {
        printf("  %s", lines[i]);
    }

    for (int i = 0; i < count; i++) {
        free(lines[i]);
    }

    return 0;
}

void hub_chat_log(hub_ctx *ctx, const char *fmt, ...)
{
    /* Only log if chat file exists */
    if (access(ctx->chat_path, F_OK) != 0) return;

    char msg[HUB_MAX_LINE - 8];  /* Leave room for "HUB:" prefix */
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    /* Prefix with HUB: for machine readability */
    char prefixed[HUB_MAX_LINE];
    snprintf(prefixed, sizeof(prefixed), "HUB:%s", msg);

    chat_send(ctx->chat_path, "hub", prefixed);
}
