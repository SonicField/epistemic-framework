/*
 * hub_commands.h — NBS Hub command implementations
 *
 * Each command function takes a hub_ctx (with manifest and state loaded)
 * and returns an exit code (0 = success).
 */

#ifndef NBS_HUB_COMMANDS_H
#define NBS_HUB_COMMANDS_H

#include "hub_state.h"

/* hub init <project-dir> <goal> */
int hub_init(hub_ctx *ctx, const char *project_dir, const char *goal);

/* hub status */
int hub_status(hub_ctx *ctx);

/* hub spawn <slug> <task-description> */
int hub_spawn(hub_ctx *ctx, int argc, char **argv);

/* hub check <worker-name> */
int hub_check(hub_ctx *ctx, const char *worker_name);

/* hub result <worker-name> */
int hub_result(hub_ctx *ctx, const char *worker_name);

/* hub dismiss <worker-name> */
int hub_dismiss(hub_ctx *ctx, const char *worker_name);

/* hub list */
int hub_list(hub_ctx *ctx);

/* hub audit <file> */
int hub_audit(hub_ctx *ctx, const char *audit_file);

/* hub gate <phase-name> <tests> <audit> */
int hub_gate(hub_ctx *ctx, int argc, char **argv);

/* hub phase */
int hub_phase(hub_ctx *ctx);

/* hub doc list */
int hub_doc_list(hub_ctx *ctx);

/* hub doc read <name> */
int hub_doc_read(hub_ctx *ctx, const char *name);

/* hub doc register <name> <path> */
int hub_doc_register(hub_ctx *ctx, const char *name, const char *path);

/* hub decision <text> */
int hub_decision(hub_ctx *ctx, const char *text);

/* hub help */
void hub_help(void);

#endif /* NBS_HUB_COMMANDS_H */
