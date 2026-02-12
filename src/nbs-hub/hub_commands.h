/*
 * hub_commands.h — NBS Hub command implementations
 *
 * Each command function takes a hub_ctx (with manifest and state loaded)
 * and returns an exit code (0 = success).
 *
 * All functions taking hub_ctx* assert ctx != NULL on entry.
 * All functions taking string parameters assert those parameters != NULL.
 * All path-building operations assert no snprintf truncation occurred.
 */

#ifndef NBS_HUB_COMMANDS_H
#define NBS_HUB_COMMANDS_H

#include "hub_state.h"

/*
 * hub_init — Initialise a new hub for a project.
 *
 * Preconditions:
 *   - ctx != NULL
 *   - project_dir != NULL (absolute or relative path to project root)
 *   - goal != NULL (non-empty terminal goal description)
 *   - No hub already exists at project_dir/.nbs/hub/
 *
 * Postconditions on success (returns 0):
 *   - Directory structure created: .nbs/hub/, .nbs/hub/audits/, .nbs/hub/gates/
 *   - Manifest and state files written to .nbs/hub/
 *   - hub.chat created at .nbs/chat/hub.chat
 *   - ctx->hub_dir, ctx->chat_path, ctx->manifest, ctx->state populated
 *   - INIT logged to hub.log and hub.chat
 *
 * Returns: 0 on success, 1 if hub already exists, -1 on I/O error.
 */
int hub_init(hub_ctx *ctx, const char *project_dir, const char *goal);

/*
 * hub_status — Display full project state.
 *
 * Preconditions:
 *   - ctx != NULL (manifest and state loaded)
 *
 * Postconditions:
 *   - Project state printed to stdout (phase, counters, workers, docs, log)
 *   - No state mutation
 *
 * Returns: 0 always.
 */
int hub_status(hub_ctx *ctx);

/*
 * hub_spawn — Spawn a new worker (enforces audit gate).
 *
 * Preconditions:
 *   - ctx != NULL (manifest and state loaded)
 *   - argv != NULL
 *   - argc >= 2 (argv[0] = slug, argv[1] = task description)
 *   - ctx->state.audit_required == 0 (else spawn is refused)
 *
 * Postconditions on success (returns 0):
 *   - Worker spawned via nbs-worker
 *   - ctx->state.total_prompts incremented
 *   - ctx->state.last_spawn_time updated
 *   - State file saved
 *   - SPAWN logged to hub.log and hub.chat
 *
 * Returns: 0 on success, 3 if audit required, 4 on usage error,
 *          non-zero on worker spawn failure.
 */
int hub_spawn(hub_ctx *ctx, int argc, char **argv);

/*
 * hub_check — Check a worker's status.
 *
 * Preconditions:
 *   - ctx != NULL
 *   - worker_name != NULL
 *
 * Postconditions:
 *   - Worker status printed to stdout via nbs-worker
 *   - No state mutation
 *
 * Returns: exit code from nbs-worker status.
 */
int hub_check(hub_ctx *ctx, const char *worker_name);

/*
 * hub_result — Retrieve worker results and update counters.
 *
 * Preconditions:
 *   - ctx != NULL (manifest and state loaded)
 *   - worker_name != NULL
 *
 * Postconditions on success (returns 0):
 *   - Worker results printed to stdout
 *   - ctx->state.workers_completed incremented
 *   - ctx->state.workers_since_check incremented
 *   - ctx->state.audit_required set to 1 if workers_since_check >= 3
 *   - State file saved
 *   - RESULT and COUNTER logged to hub.log
 *
 * On nbs-worker failure (non-zero return):
 *   - State is NOT mutated (counters not updated)
 *   - Warning printed to stderr
 *
 * Returns: 0 on success, non-zero on worker results failure, -1 on save error.
 */
int hub_result(hub_ctx *ctx, const char *worker_name);

/*
 * hub_dismiss — Dismiss a worker.
 *
 * Preconditions:
 *   - ctx != NULL
 *   - worker_name != NULL
 *
 * Postconditions on success (returns 0):
 *   - Worker dismissed via nbs-worker
 *   - DISMISS logged to hub.log
 *
 * Returns: exit code from nbs-worker dismiss.
 */
int hub_dismiss(hub_ctx *ctx, const char *worker_name);

/*
 * hub_list — List all workers.
 *
 * Preconditions:
 *   - ctx != NULL
 *
 * Postconditions:
 *   - Worker list printed to stdout via nbs-worker
 *   - No state mutation
 *
 * Returns: exit code from nbs-worker list.
 */
int hub_list(hub_ctx *ctx);

/*
 * hub_audit — Submit an NBS self-check audit.
 *
 * Preconditions:
 *   - ctx != NULL (manifest and state loaded)
 *   - audit_file != NULL (path to non-empty audit file)
 *   - Audit file must contain references to: goal/terminal, delegation/workers, learnings/3Ws
 *
 * Postconditions on success (returns 0):
 *   - Audit file copied to .nbs/hub/audits/audit-NNN.md
 *   - ctx->state.workers_since_check reset to 0
 *   - ctx->state.audit_required reset to 0
 *   - ctx->state.last_audit_time updated
 *   - State file saved
 *   - AUDIT logged to hub.log and hub.chat
 *
 * Returns: 0 on success, 1 on validation/archive failure, -1 on save error.
 */
int hub_audit(hub_ctx *ctx, const char *audit_file);

/*
 * hub_gate — Submit a phase gate.
 *
 * Preconditions:
 *   - ctx != NULL (manifest and state loaded)
 *   - argv != NULL
 *   - argc >= 3 (argv[0] = phase_name, argv[1] = test_file, argv[2] = audit_file)
 *   - phase_name must match ctx->state.phase_name (no phase skipping)
 *   - test_file and audit_file must be non-empty
 *
 * Postconditions on success (returns 0):
 *   - Gate record written to .nbs/hub/gates/phase-N-gate.md
 *   - ctx->state.phase incremented
 *   - ctx->state.phase_name and phase_gate_criteria cleared
 *   - Audit counters reset
 *   - State file saved
 *   - GATE_PASSED logged to hub.log and hub.chat
 *
 * Returns: 0 on success, 1 on validation/write failure, 4 on usage error,
 *          -1 on save error.
 */
int hub_gate(hub_ctx *ctx, int argc, char **argv);

/*
 * hub_phase — Display current phase information.
 *
 * Preconditions:
 *   - ctx != NULL (state loaded)
 *
 * Postconditions:
 *   - Phase info printed to stdout
 *   - No state mutation
 *
 * Returns: 0 always.
 */
int hub_phase(hub_ctx *ctx);

/*
 * hub_doc_list — List registered documents.
 *
 * Preconditions:
 *   - ctx != NULL (manifest loaded)
 *
 * Postconditions:
 *   - Document list printed to stdout (with [MISSING] for absent files)
 *   - No state mutation
 *
 * Returns: 0 always.
 */
int hub_doc_list(hub_ctx *ctx);

/*
 * hub_doc_read — Output contents of a registered document.
 *
 * Preconditions:
 *   - ctx != NULL (manifest loaded)
 *   - name != NULL (document name to look up)
 *
 * Postconditions on success (returns 0):
 *   - Document contents printed to stdout
 *   - No state mutation
 *
 * Returns: 0 on success, 1 if file missing/unreadable, 2 if name not registered.
 */
int hub_doc_read(hub_ctx *ctx, const char *name);

/*
 * hub_doc_register — Register or update a document.
 *
 * Preconditions:
 *   - ctx != NULL (manifest loaded)
 *   - name != NULL (document name)
 *   - path != NULL (file path, absolute or relative)
 *   - ctx->manifest.doc_count < HUB_MAX_DOCS (for new registrations)
 *
 * Postconditions on success (returns 0):
 *   - Document entry added to or updated in ctx->manifest.docs
 *   - Manifest file saved
 *   - DOC_REGISTER or DOC_UPDATE logged to hub.log
 *
 * Returns: 0 on success, -1 on save error.
 */
int hub_doc_register(hub_ctx *ctx, const char *name, const char *path);

/*
 * hub_decision — Record a decision.
 *
 * Preconditions:
 *   - ctx != NULL
 *   - text != NULL (decision text)
 *
 * Postconditions:
 *   - DECISION logged to hub.log and hub.chat
 *   - No state mutation
 *
 * Returns: 0 always.
 */
int hub_decision(hub_ctx *ctx, const char *text);

/*
 * hub_help — Print usage information.
 *
 * No preconditions.
 * Postcondition: help text printed to stdout.
 */
void hub_help(void);

#endif /* NBS_HUB_COMMANDS_H */
