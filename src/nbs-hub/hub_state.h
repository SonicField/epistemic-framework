/*
 * hub_state.h — NBS Hub state structures and file I/O
 *
 * The hub's state is entirely file-based: a manifest (project identity
 * and document registry) and a state file (counters, flags, phase tracking).
 * These structs are the in-memory representation, populated by reading
 * the files at startup and written back atomically on mutation.
 *
 * Invariants (enforced by ASSERT_MSG in hub_state.c):
 *   - doc_count in [0, HUB_MAX_DOCS]
 *   - workers_since_check >= 0
 *   - audit_required is 0 or 1
 *   - phase >= 0
 *   - All path/string buffers are null-terminated within their declared size
 *   - All integer fields parsed via strtol with range checking (not atoi)
 */

#ifndef NBS_HUB_STATE_H
#define NBS_HUB_STATE_H

#include <stdint.h>
#include <time.h>
#include "chat_file.h"  /* ASSERT_MSG */

/* Maximum limits — fixed at compile time */
#define HUB_MAX_DOCS          64
#define HUB_MAX_PATH        4096
#define HUB_MAX_NAME          128
#define HUB_MAX_GOAL         1024
#define HUB_MAX_GATE_CRITERIA 1024
#define HUB_MAX_LINE         8192

/* Document registry entry */
typedef struct {
    char name[HUB_MAX_NAME];   /* non-empty, null-terminated, max HUB_MAX_NAME-1 chars */
    char path[HUB_MAX_PATH];   /* non-empty, null-terminated, max HUB_MAX_PATH-1 chars */
} hub_doc_entry;

/* Manifest — project identity and document registry */
typedef struct {
    char project_dir[HUB_MAX_PATH];   /* absolute path, non-empty after load */
    char terminal_goal[HUB_MAX_GOAL]; /* non-empty after load */
    char workers_dir[HUB_MAX_PATH];   /* absolute path, may be empty if unset */
    char hub_dir[HUB_MAX_PATH];       /* absolute path, may be empty if unset */
    hub_doc_entry docs[HUB_MAX_DOCS]; /* populated entries: docs[0..doc_count-1] */
    int doc_count;                     /* range: [0, HUB_MAX_DOCS] */
} hub_manifest;

/* State — counters, flags, phase tracking */
typedef struct {
    int phase;                  /* >= 0; monotonically increasing phase number */
    char phase_name[HUB_MAX_NAME];  /* human-readable phase label */
    char phase_gate_criteria[HUB_MAX_GATE_CRITERIA]; /* gate pass criteria text */
    int total_prompts;          /* >= 0; cumulative prompt count */
    int workers_completed;      /* >= 0; cumulative completed worker count */
    int workers_since_check;    /* >= 0; workers since last audit check */
    int audit_required;         /* boolean: 0 or 1 only */
    time_t last_audit_time;     /* 0 if never audited; otherwise UTC epoch seconds */
    time_t last_spawn_time;     /* 0 if never spawned; otherwise UTC epoch seconds */
    int stall_threshold_seconds; /* > 0; seconds of inactivity before stall warning */
} hub_state;

/* Top-level hub context — passed to all command functions */
typedef struct {
    hub_manifest manifest;
    hub_state state;
    char hub_dir[HUB_MAX_PATH];    /* .nbs/hub/ absolute path, non-empty after discover */
    char chat_path[HUB_MAX_PATH];  /* .nbs/chat/hub.chat, non-empty after discover */
    int log_fd;                     /* open fd for hub.log (O_APPEND), -1 if not open */
} hub_ctx;

/*
 * hub_discover — Find and load hub state from the given directory.
 *
 * Searches upward from search_dir for .nbs/hub/. If found, loads
 * manifest and state files into ctx. If not found, prints a
 * HUB-QUESTION and returns 2.
 *
 * Preconditions:
 *   - ctx != NULL (asserted)
 *   - search_dir != NULL (asserted)
 *
 * Postconditions (on success, return 0):
 *   - ctx->hub_dir is set to the absolute path of .nbs/hub/
 *   - ctx->chat_path is set to the absolute path of hub.chat
 *   - ctx->manifest and ctx->state are fully populated
 *
 * Returns 0 on success, 2 if not found, -1 on load error.
 */
int hub_discover(hub_ctx *ctx, const char *search_dir);

/*
 * hub_load_manifest — Read manifest file into ctx->manifest.
 *
 * Preconditions:
 *   - ctx != NULL (asserted)
 *   - ctx->hub_dir is set to a valid .nbs/hub/ path
 *
 * Postconditions (on success, return 0):
 *   - ctx->manifest.project_dir is non-empty (asserted)
 *   - ctx->manifest.terminal_goal is non-empty (asserted)
 *   - ctx->manifest.doc_count in [0, HUB_MAX_DOCS] (asserted)
 *
 * Returns 0 on success, -1 on error.
 */
int hub_load_manifest(hub_ctx *ctx);

/*
 * hub_save_manifest — Write ctx->manifest to manifest file atomically.
 *
 * Writes to a temp file, then rename(). Checks fprintf and fclose
 * return values; cleans up temp file on I/O failure.
 *
 * Preconditions:
 *   - ctx != NULL (asserted)
 *   - ctx->hub_dir is set to a valid .nbs/hub/ path
 *
 * Returns 0 on success, -1 on error (with diagnostic on stderr).
 */
int hub_save_manifest(hub_ctx *ctx);

/*
 * hub_load_state — Read state file into ctx->state.
 *
 * All integer fields are parsed with strtol and range-checked.
 * Malformed values produce a warning on stderr and leave the
 * field at its default (zero or the previous default).
 *
 * Preconditions:
 *   - ctx != NULL (asserted)
 *   - ctx->hub_dir is set to a valid .nbs/hub/ path
 *
 * Postconditions (on success, return 0):
 *   - ctx->state.phase >= 0 (asserted)
 *   - ctx->state.workers_since_check >= 0 (asserted)
 *   - ctx->state.audit_required is 0 or 1 (asserted)
 *
 * Returns 0 on success, -1 on error.
 */
int hub_load_state(hub_ctx *ctx);

/*
 * hub_save_state — Write ctx->state to state file atomically.
 *
 * Writes to a temp file, then rename(). Checks fprintf and fclose
 * return values; cleans up temp file on I/O failure.
 *
 * Preconditions:
 *   - ctx != NULL (asserted)
 *   - ctx->hub_dir is set to a valid .nbs/hub/ path
 *
 * Returns 0 on success, -1 on error (with diagnostic on stderr).
 */
int hub_save_state(hub_ctx *ctx);

/*
 * hub_create_dirs — Create the .nbs/hub/ directory structure.
 *
 * Creates: .nbs/, .nbs/hub/, .nbs/hub/audits/, .nbs/hub/gates/,
 *          .nbs/chat/ (if not exists).
 *
 * Preconditions:
 *   - project_dir != NULL, non-empty
 *
 * Returns 0 on success, -1 on error (with diagnostic on stderr).
 */
int hub_create_dirs(const char *project_dir);

/*
 * format_time — Format a time_t as ISO 8601 (YYYY-MM-DDTHH:MM:SS).
 *
 * WARNING: Returns a pointer to a static buffer. Not thread-safe.
 * Each call overwrites the previous result. Callers must copy the
 * result if it needs to persist across multiple format_time calls.
 *
 * Preconditions:
 *   - If t != 0, gmtime_r(t) must succeed (asserted)
 *
 * Postconditions:
 *   - Returns a null-terminated string
 *   - If t == 0, returns an empty string ""
 *   - If t != 0, returns ISO 8601 formatted UTC time
 *
 * Round-trip property: for any valid t != 0,
 *   parse_time(format_time(t)) == t
 * This property is enforced by the test suite.
 */
const char *format_time(time_t t);

/*
 * parse_time — Parse an ISO 8601 string (YYYY-MM-DDTHH:MM:SS) to time_t.
 *
 * Preconditions:
 *   - s may be NULL or empty (both return 0)
 *
 * Postconditions:
 *   - Returns 0 on empty/missing/unparseable string
 *   - Returns a positive time_t on successful parse
 *
 * Round-trip property: for any valid t != 0,
 *   parse_time(format_time(t)) == t
 * This property is enforced by the test suite.
 */
time_t parse_time(const char *s);

#endif /* NBS_HUB_STATE_H */
