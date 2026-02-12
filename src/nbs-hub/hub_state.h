/*
 * hub_state.h — NBS Hub state structures and file I/O
 *
 * The hub's state is entirely file-based: a manifest (project identity
 * and document registry) and a state file (counters, flags, phase tracking).
 * These structs are the in-memory representation, populated by reading
 * the files at startup and written back atomically on mutation.
 *
 * Invariants:
 *   - doc_count <= HUB_MAX_DOCS
 *   - workers_since_check >= 0
 *   - audit_required is 0 or 1
 *   - phase >= 0
 *   - All path buffers are null-terminated
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
    char name[HUB_MAX_NAME];
    char path[HUB_MAX_PATH];
} hub_doc_entry;

/* Manifest — project identity and document registry */
typedef struct {
    char project_dir[HUB_MAX_PATH];
    char terminal_goal[HUB_MAX_GOAL];
    char workers_dir[HUB_MAX_PATH];
    char hub_dir[HUB_MAX_PATH];
    hub_doc_entry docs[HUB_MAX_DOCS];
    int doc_count;
} hub_manifest;

/* State — counters, flags, phase tracking */
typedef struct {
    int phase;
    char phase_name[HUB_MAX_NAME];
    char phase_gate_criteria[HUB_MAX_GATE_CRITERIA];
    int total_prompts;
    int workers_completed;
    int workers_since_check;
    int audit_required;       /* 0 or 1 */
    time_t last_audit_time;   /* 0 if never */
    time_t last_spawn_time;   /* 0 if never */
    int stall_threshold_seconds;
} hub_state;

/* Top-level hub context — passed to all command functions */
typedef struct {
    hub_manifest manifest;
    hub_state state;
    char hub_dir[HUB_MAX_PATH];    /* .nbs/hub/ absolute path */
    char chat_path[HUB_MAX_PATH];  /* .nbs/chat/hub.chat */
    int log_fd;                     /* open fd for hub.log (O_APPEND) */
} hub_ctx;

/*
 * hub_discover — Find and load hub state from the given directory.
 *
 * Searches upward from search_dir for .nbs/hub/. If found, loads
 * manifest and state files into ctx. If not found, prints a
 * HUB-QUESTION and returns 2.
 *
 * Returns 0 on success, 2 if not found.
 */
int hub_discover(hub_ctx *ctx, const char *search_dir);

/*
 * hub_load_manifest — Read manifest file into ctx->manifest.
 *
 * Expects ctx->hub_dir to be set. Returns 0 on success, -1 on error.
 */
int hub_load_manifest(hub_ctx *ctx);

/*
 * hub_save_manifest — Write ctx->manifest to manifest file atomically.
 *
 * Writes to a temp file, then rename(). Returns 0 on success, -1 on error.
 */
int hub_save_manifest(hub_ctx *ctx);

/*
 * hub_load_state — Read state file into ctx->state.
 *
 * Expects ctx->hub_dir to be set. Returns 0 on success, -1 on error.
 */
int hub_load_state(hub_ctx *ctx);

/*
 * hub_save_state — Write ctx->state to state file atomically.
 *
 * Writes to a temp file, then rename(). Returns 0 on success, -1 on error.
 */
int hub_save_state(hub_ctx *ctx);

/*
 * hub_create_dirs — Create the .nbs/hub/ directory structure.
 *
 * Creates: .nbs/, .nbs/hub/, .nbs/hub/audits/, .nbs/hub/gates/,
 *          .nbs/chat/ (if not exists).
 * Returns 0 on success, -1 on error.
 */
int hub_create_dirs(const char *project_dir);

/*
 * format_time — Format a time_t as ISO 8601.
 *
 * Returns a pointer to a static buffer. Not thread-safe.
 */
const char *format_time(time_t t);

/*
 * parse_time — Parse an ISO 8601 string to time_t.
 *
 * Returns 0 on empty/missing string.
 */
time_t parse_time(const char *s);

#endif /* NBS_HUB_STATE_H */
