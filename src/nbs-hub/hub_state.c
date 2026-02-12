/*
 * hub_state.c — NBS Hub state file I/O
 *
 * Handles reading and writing of manifest and state files.
 * All writes are atomic: write to temp file, then rename().
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <time.h>

#include "hub_state.h"

/* Safe string copy — always null-terminates, truncates if src exceeds dst_sz */
static void safe_copy(char *dst, size_t dst_sz, const char *src)
{
    ASSERT_MSG(dst != NULL, "safe_copy: dst is NULL");
    ASSERT_MSG(src != NULL, "safe_copy: src is NULL");
    ASSERT_MSG(dst_sz > 0,
               "safe_copy: dst_sz is 0 — would cause size_t underflow in truncation logic");
    size_t len = strlen(src);
    if (len >= dst_sz) len = dst_sz - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}

/*
 * safe_parse_int — Parse a string to int with overflow and format checking.
 *
 * Returns 0 on success (value written to *out), -1 on parse failure.
 * Rejects empty strings, trailing non-whitespace, and values outside INT range.
 */
static int safe_parse_int(const char *str, int *out)
{
    ASSERT_MSG(str != NULL, "safe_parse_int: str is NULL");
    ASSERT_MSG(out != NULL, "safe_parse_int: out is NULL");
    char *endptr;
    errno = 0;
    long val = strtol(str, &endptr, 10);
    if (errno != 0 || endptr == str ||
        (*endptr != '\0' && *endptr != '\n' && *endptr != '\r'))
        return -1;
    if (val < INT_MIN || val > INT_MAX)
        return -1;
    *out = (int)val;
    return 0;
}

/* ── Time formatting ──────────────────────────────────────────── */

const char *format_time(time_t t)
{
    static char buf[32];
    if (t == 0) {
        buf[0] = '\0';
        return buf;
    }
    struct tm tm;
    struct tm *result = gmtime_r(&t, &tm);
    ASSERT_MSG(result != NULL,
               "format_time: gmtime_r failed for time_t %ld", (long)t);
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    return buf;
}

time_t parse_time(const char *s)
{
    if (!s || s[0] == '\0') return 0;
    struct tm tm = {0};
    if (strptime(s, "%Y-%m-%dT%H:%M:%S", &tm) == NULL) return 0;
    return timegm(&tm);
}

/* ── Directory creation ───────────────────────────────────────── */

static int mkdir_p(const char *path, mode_t mode)
{
    char tmp[HUB_MAX_PATH];
    int len = snprintf(tmp, sizeof(tmp), "%s", path);
    ASSERT_MSG(len > 0 && len < (int)sizeof(tmp), "path too long: %s", path);

    /* Strip trailing slash */
    if (tmp[len - 1] == '/') tmp[len - 1] = '\0';

    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, mode) != 0 && errno != EEXIST) return -1;
            *p = '/';
        }
    }
    if (mkdir(tmp, mode) != 0 && errno != EEXIST) return -1;
    return 0;
}

int hub_create_dirs(const char *project_dir)
{
    char path[HUB_MAX_PATH];
    int n;

    n = snprintf(path, sizeof(path), "%s/.nbs/hub/audits", project_dir);
    ASSERT_MSG(n > 0 && n < (int)sizeof(path), "path overflow");
    if (mkdir_p(path, 0755) != 0) {
        fprintf(stderr, "error: cannot create %s: %s\n", path, strerror(errno));
        return -1;
    }

    n = snprintf(path, sizeof(path), "%s/.nbs/hub/gates", project_dir);
    ASSERT_MSG(n > 0 && n < (int)sizeof(path), "path overflow");
    if (mkdir_p(path, 0755) != 0) {
        fprintf(stderr, "error: cannot create %s: %s\n", path, strerror(errno));
        return -1;
    }

    n = snprintf(path, sizeof(path), "%s/.nbs/chat", project_dir);
    ASSERT_MSG(n > 0 && n < (int)sizeof(path), "path overflow");
    if (mkdir_p(path, 0755) != 0) {
        fprintf(stderr, "error: cannot create %s: %s\n", path, strerror(errno));
        return -1;
    }

    return 0;
}

/* ── Key=value file parsing ───────────────────────────────────── */

/* Parse a single key=value line. Returns 1 if parsed, 0 if comment/blank. */
static int parse_kv_line(const char *line, char *key, size_t key_sz,
                         char *value, size_t val_sz)
{
    /* Skip comments and blank lines */
    while (*line == ' ' || *line == '\t') line++;
    if (*line == '#' || *line == '\n' || *line == '\0') return 0;

    const char *eq = strchr(line, '=');
    if (!eq) return 0;

    size_t klen = (size_t)(eq - line);
    if (klen >= key_sz) klen = key_sz - 1;
    memcpy(key, line, klen);
    key[klen] = '\0';

    const char *vstart = eq + 1;
    size_t vlen = strlen(vstart);
    /* Strip trailing newline */
    while (vlen > 0 && (vstart[vlen - 1] == '\n' || vstart[vlen - 1] == '\r'))
        vlen--;
    if (vlen >= val_sz) vlen = val_sz - 1;
    memcpy(value, vstart, vlen);
    value[vlen] = '\0';

    return 1;
}

/* ── Manifest I/O ─────────────────────────────────────────────── */

int hub_load_manifest(hub_ctx *ctx)
{
    ASSERT_MSG(ctx != NULL,
               "hub_load_manifest: ctx is NULL — caller must provide valid hub context");
    char path[HUB_MAX_PATH];
    int n = snprintf(path, sizeof(path), "%s/manifest", ctx->hub_dir);
    ASSERT_MSG(n > 0 && n < (int)sizeof(path), "path overflow");

    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "error: cannot open manifest: %s\n", strerror(errno));
        return -1;
    }

    hub_manifest *m = &ctx->manifest;
    memset(m, 0, sizeof(*m));

    char line[HUB_MAX_LINE];
    char key[HUB_MAX_NAME];
    char value[HUB_MAX_PATH];

    while (fgets(line, sizeof(line), fp)) {
        if (!parse_kv_line(line, key, sizeof(key), value, sizeof(value)))
            continue;

        if (strcmp(key, "project_dir") == 0) {
            safe_copy(m->project_dir, sizeof(m->project_dir), value);
        } else if (strcmp(key, "terminal_goal") == 0) {
            safe_copy(m->terminal_goal, sizeof(m->terminal_goal), value);
        } else if (strcmp(key, "workers_dir") == 0) {
            safe_copy(m->workers_dir, sizeof(m->workers_dir), value);
        } else if (strcmp(key, "hub_dir") == 0) {
            safe_copy(m->hub_dir, sizeof(m->hub_dir), value);
        } else if (strncmp(key, "doc.", 4) == 0) {
            ASSERT_MSG(m->doc_count < HUB_MAX_DOCS,
                       "too many docs: %d >= %d", m->doc_count, HUB_MAX_DOCS);
            hub_doc_entry *e = &m->docs[m->doc_count];
            safe_copy(e->name, sizeof(e->name), key + 4);
            safe_copy(e->path, sizeof(e->path), value);
            m->doc_count++;
        }
    }

    fclose(fp);

    /* Postconditions */
    ASSERT_MSG(m->project_dir[0] != '\0', "manifest missing project_dir");
    ASSERT_MSG(m->terminal_goal[0] != '\0', "manifest missing terminal_goal");
    ASSERT_MSG(m->doc_count >= 0 && m->doc_count <= HUB_MAX_DOCS,
               "doc_count out of range: %d", m->doc_count);

    return 0;
}

int hub_save_manifest(hub_ctx *ctx)
{
    ASSERT_MSG(ctx != NULL,
               "hub_save_manifest: ctx is NULL — caller must provide valid hub context");
    char path[HUB_MAX_PATH];
    char tmp_path[HUB_MAX_PATH];
    int n;

    n = snprintf(path, sizeof(path), "%s/manifest", ctx->hub_dir);
    ASSERT_MSG(n > 0 && n < (int)sizeof(path), "path overflow");
    n = snprintf(tmp_path, sizeof(tmp_path), "%s/manifest.tmp", ctx->hub_dir);
    ASSERT_MSG(n > 0 && n < (int)sizeof(tmp_path), "path overflow");

    FILE *fp = fopen(tmp_path, "w");
    if (!fp) {
        fprintf(stderr, "error: cannot write manifest: %s\n", strerror(errno));
        return -1;
    }

    hub_manifest *m = &ctx->manifest;
    int write_err = 0;

    if (fprintf(fp, "# NBS Hub Manifest\n") < 0) write_err = 1;
    if (fprintf(fp, "# Auto-generated by: nbs-hub\n\n") < 0) write_err = 1;
    if (fprintf(fp, "project_dir=%s\n", m->project_dir) < 0) write_err = 1;
    if (fprintf(fp, "terminal_goal=%s\n", m->terminal_goal) < 0) write_err = 1;
    if (fprintf(fp, "workers_dir=%s\n", m->workers_dir) < 0) write_err = 1;
    if (fprintf(fp, "hub_dir=%s\n", m->hub_dir) < 0) write_err = 1;
    if (fprintf(fp, "\n# Document registry\n") < 0) write_err = 1;

    for (int i = 0; i < m->doc_count; i++) {
        if (fprintf(fp, "doc.%s=%s\n", m->docs[i].name, m->docs[i].path) < 0)
            write_err = 1;
    }

    if (write_err) {
        fprintf(stderr, "error: write failed for manifest: %s\n", strerror(errno));
        fclose(fp);
        unlink(tmp_path);
        return -1;
    }

    if (fclose(fp) != 0) {
        fprintf(stderr, "error: fclose failed writing manifest: %s\n", strerror(errno));
        unlink(tmp_path);
        return -1;
    }

    if (rename(tmp_path, path) != 0) {
        fprintf(stderr, "error: rename manifest: %s\n", strerror(errno));
        unlink(tmp_path);
        return -1;
    }

    return 0;
}

/* ── State I/O ────────────────────────────────────────────────── */

int hub_load_state(hub_ctx *ctx)
{
    ASSERT_MSG(ctx != NULL,
               "hub_load_state: ctx is NULL — caller must provide valid hub context");
    char path[HUB_MAX_PATH];
    int n = snprintf(path, sizeof(path), "%s/state", ctx->hub_dir);
    ASSERT_MSG(n > 0 && n < (int)sizeof(path), "path overflow");

    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "error: cannot open state: %s\n", strerror(errno));
        return -1;
    }

    hub_state *s = &ctx->state;
    memset(s, 0, sizeof(*s));
    s->stall_threshold_seconds = 1800;  /* default 30 min */

    char line[HUB_MAX_LINE];
    char key[HUB_MAX_NAME];
    char value[HUB_MAX_GATE_CRITERIA];

    while (fgets(line, sizeof(line), fp)) {
        if (!parse_kv_line(line, key, sizeof(key), value, sizeof(value)))
            continue;

        if (strcmp(key, "phase") == 0) {
            if (safe_parse_int(value, &s->phase) != 0)
                fprintf(stderr, "warning: malformed integer for 'phase': %s\n", value);
        } else if (strcmp(key, "phase_name") == 0)
            safe_copy(s->phase_name, sizeof(s->phase_name), value);
        else if (strcmp(key, "phase_gate_criteria") == 0)
            safe_copy(s->phase_gate_criteria, sizeof(s->phase_gate_criteria), value);
        else if (strcmp(key, "total_prompts") == 0) {
            if (safe_parse_int(value, &s->total_prompts) != 0)
                fprintf(stderr, "warning: malformed integer for 'total_prompts': %s\n", value);
        } else if (strcmp(key, "workers_completed") == 0) {
            if (safe_parse_int(value, &s->workers_completed) != 0)
                fprintf(stderr, "warning: malformed integer for 'workers_completed': %s\n", value);
        } else if (strcmp(key, "workers_since_check") == 0) {
            if (safe_parse_int(value, &s->workers_since_check) != 0)
                fprintf(stderr, "warning: malformed integer for 'workers_since_check': %s\n", value);
        } else if (strcmp(key, "audit_required") == 0) {
            if (safe_parse_int(value, &s->audit_required) != 0)
                fprintf(stderr, "warning: malformed integer for 'audit_required': %s\n", value);
        } else if (strcmp(key, "last_audit_time") == 0)
            s->last_audit_time = parse_time(value);
        else if (strcmp(key, "last_spawn_time") == 0)
            s->last_spawn_time = parse_time(value);
        else if (strcmp(key, "stall_threshold_seconds") == 0) {
            if (safe_parse_int(value, &s->stall_threshold_seconds) != 0)
                fprintf(stderr, "warning: malformed integer for 'stall_threshold_seconds': %s\n", value);
        }
    }

    fclose(fp);

    /* Postconditions */
    ASSERT_MSG(s->phase >= 0, "phase negative: %d", s->phase);
    ASSERT_MSG(s->workers_since_check >= 0,
               "workers_since_check negative: %d", s->workers_since_check);
    ASSERT_MSG(s->audit_required == 0 || s->audit_required == 1,
               "audit_required not boolean: %d", s->audit_required);

    return 0;
}

int hub_save_state(hub_ctx *ctx)
{
    ASSERT_MSG(ctx != NULL,
               "hub_save_state: ctx is NULL — caller must provide valid hub context");
    char path[HUB_MAX_PATH];
    char tmp_path[HUB_MAX_PATH];
    int n;

    n = snprintf(path, sizeof(path), "%s/state", ctx->hub_dir);
    ASSERT_MSG(n > 0 && n < (int)sizeof(path), "path overflow");
    n = snprintf(tmp_path, sizeof(tmp_path), "%s/state.tmp", ctx->hub_dir);
    ASSERT_MSG(n > 0 && n < (int)sizeof(tmp_path), "path overflow");

    FILE *fp = fopen(tmp_path, "w");
    if (!fp) {
        fprintf(stderr, "error: cannot write state: %s\n", strerror(errno));
        return -1;
    }

    hub_state *s = &ctx->state;
    int write_err = 0;

    if (fprintf(fp, "# NBS Hub State\n") < 0) write_err = 1;
    if (fprintf(fp, "# Updated atomically by hub commands\n\n") < 0) write_err = 1;
    if (fprintf(fp, "phase=%d\n", s->phase) < 0) write_err = 1;
    if (fprintf(fp, "phase_name=%s\n", s->phase_name) < 0) write_err = 1;
    if (fprintf(fp, "phase_gate_criteria=%s\n", s->phase_gate_criteria) < 0) write_err = 1;
    if (fprintf(fp, "total_prompts=%d\n", s->total_prompts) < 0) write_err = 1;
    if (fprintf(fp, "workers_completed=%d\n", s->workers_completed) < 0) write_err = 1;
    if (fprintf(fp, "workers_since_check=%d\n", s->workers_since_check) < 0) write_err = 1;
    if (fprintf(fp, "audit_required=%d\n", s->audit_required) < 0) write_err = 1;
    if (fprintf(fp, "last_audit_time=%s\n", format_time(s->last_audit_time)) < 0) write_err = 1;
    if (fprintf(fp, "last_spawn_time=%s\n", format_time(s->last_spawn_time)) < 0) write_err = 1;
    if (fprintf(fp, "stall_threshold_seconds=%d\n", s->stall_threshold_seconds) < 0) write_err = 1;

    if (write_err) {
        fprintf(stderr, "error: write failed for state: %s\n", strerror(errno));
        fclose(fp);
        unlink(tmp_path);
        return -1;
    }

    if (fclose(fp) != 0) {
        fprintf(stderr, "error: fclose failed writing state: %s\n", strerror(errno));
        unlink(tmp_path);
        return -1;
    }

    if (rename(tmp_path, path) != 0) {
        fprintf(stderr, "error: rename state: %s\n", strerror(errno));
        unlink(tmp_path);
        return -1;
    }

    return 0;
}

/* ── Discovery ────────────────────────────────────────────────── */

int hub_discover(hub_ctx *ctx, const char *search_dir)
{
    ASSERT_MSG(ctx != NULL,
               "hub_discover: ctx is NULL — caller must provide valid hub context");
    ASSERT_MSG(search_dir != NULL,
               "hub_discover: search_dir is NULL — caller must provide a directory to search");
    char abs_dir[HUB_MAX_PATH];

    /* Resolve to absolute path */
    if (search_dir[0] == '/') {
        safe_copy(abs_dir, sizeof(abs_dir), search_dir);
    } else {
        char *r = realpath(search_dir, abs_dir);
        if (!r) {
            fprintf(stderr, "error: cannot resolve path: %s\n", search_dir);
            return 2;
        }
    }

    /* Walk upward looking for .nbs/hub/ */
    char candidate[HUB_MAX_PATH];
    char *dir = abs_dir;
    struct stat st;

    while (dir[0] != '\0') {
        int n = snprintf(candidate, sizeof(candidate), "%s/.nbs/hub", dir);
        ASSERT_MSG(n > 0 && n < (int)sizeof(candidate), "path overflow");

        if (stat(candidate, &st) == 0 && S_ISDIR(st.st_mode)) {
            /* Found it */
            safe_copy(ctx->hub_dir, sizeof(ctx->hub_dir), candidate);

            /* Set chat path */
            n = snprintf(ctx->chat_path, sizeof(ctx->chat_path),
                         "%s/.nbs/chat/hub.chat", dir);
            ASSERT_MSG(n > 0 && n < (int)sizeof(ctx->chat_path), "path overflow");

            /* Load state */
            int rc = hub_load_manifest(ctx);
            if (rc != 0) return rc;
            rc = hub_load_state(ctx);
            if (rc != 0) return rc;

            /* Stall detection */
            if (ctx->state.last_spawn_time > 0) {
                time_t now = time(NULL);
                int elapsed = (int)(now - ctx->state.last_spawn_time);
                if (elapsed > ctx->state.stall_threshold_seconds &&
                    strcmp(ctx->state.phase_name, "COMPLETE") != 0) {
                    printf("[HUB-WARNING] No activity for %d minutes.\n",
                           elapsed / 60);
                    printf("  Last spawn: %s\n",
                           format_time(ctx->state.last_spawn_time));
                    printf("  Phase: %d (%s)\n",
                           ctx->state.phase, ctx->state.phase_name);
                    printf("  Are you a new session? Run: nbs-hub status\n");
                }
            }

            return 0;
        }

        /* Move up one directory */
        char *slash = strrchr(dir, '/');
        if (!slash || slash == dir) break;
        *slash = '\0';
    }

    printf("[HUB-QUESTION] No hub state found.\n");
    printf("  To initialise: nbs-hub init <project-dir> <goal>\n");
    printf("  To point to existing project: nbs-hub --project <path> status\n");
    return 2;
}
