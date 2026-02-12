/*
 * nbs-hub.c — NBS Teams process enforcement hub
 *
 * A deterministic, non-intelligent process that counts, routes, and
 * enforces NBS process discipline. All state is in files.
 *
 * See hub_commands.h for the full command API.
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hub_state.h"
#include "hub_commands.h"
#include "hub_log.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        hub_help();
        return 0;
    }

    /* Handle --project flag */
    const char *project_override = NULL;
    int arg_start = 1;

    if (strcmp(argv[1], "--project") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: nbs-hub --project <path> <command> [args...]\n");
            return 4;
        }
        project_override = argv[2];
        arg_start = 3;
    }

    const char *cmd = argv[arg_start];
    hub_ctx ctx = {0};
    ctx.log_fd = -1;

    /* Commands that don't need existing state */
    if (strcmp(cmd, "init") == 0) {
        if (argc - arg_start < 3) {
            fprintf(stderr, "Usage: nbs-hub init <project-dir> <goal>\n");
            return 4;
        }
        return hub_init(&ctx, argv[arg_start + 1], argv[arg_start + 2]);
    }
    if (strcmp(cmd, "help") == 0) {
        hub_help();
        return 0;
    }

    /* All other commands require state discovery */
    const char *search_dir = project_override ? project_override : ".";
    int rc = hub_discover(&ctx, search_dir);
    if (rc != 0) return rc;

    if (strcmp(cmd, "status") == 0)
        return hub_status(&ctx);

    if (strcmp(cmd, "spawn") == 0)
        return hub_spawn(&ctx, argc - arg_start - 1, argv + arg_start + 1);

    if (strcmp(cmd, "check") == 0) {
        if (argc - arg_start < 2) {
            fprintf(stderr, "Usage: nbs-hub check <worker-name>\n");
            return 4;
        }
        return hub_check(&ctx, argv[arg_start + 1]);
    }

    if (strcmp(cmd, "result") == 0) {
        if (argc - arg_start < 2) {
            fprintf(stderr, "Usage: nbs-hub result <worker-name>\n");
            return 4;
        }
        return hub_result(&ctx, argv[arg_start + 1]);
    }

    if (strcmp(cmd, "dismiss") == 0) {
        if (argc - arg_start < 2) {
            fprintf(stderr, "Usage: nbs-hub dismiss <worker-name>\n");
            return 4;
        }
        return hub_dismiss(&ctx, argv[arg_start + 1]);
    }

    if (strcmp(cmd, "list") == 0)
        return hub_list(&ctx);

    if (strcmp(cmd, "audit") == 0) {
        if (argc - arg_start < 2) {
            fprintf(stderr, "Usage: nbs-hub audit <file>\n");
            return 4;
        }
        return hub_audit(&ctx, argv[arg_start + 1]);
    }

    if (strcmp(cmd, "gate") == 0)
        return hub_gate(&ctx, argc - arg_start - 1, argv + arg_start + 1);

    if (strcmp(cmd, "phase") == 0)
        return hub_phase(&ctx);

    if (strcmp(cmd, "log") == 0) {
        int n = 20;
        if (argc - arg_start >= 2)
            n = atoi(argv[arg_start + 1]);
        if (hub_log_open(&ctx) != 0) return 1;
        rc = hub_log_show(&ctx, n);
        hub_log_close(&ctx);
        return rc;
    }

    if (strcmp(cmd, "decision") == 0) {
        if (argc - arg_start < 2) {
            fprintf(stderr, "Usage: nbs-hub decision <text>\n");
            return 4;
        }
        return hub_decision(&ctx, argv[arg_start + 1]);
    }

    /* doc subcommands */
    if (strcmp(cmd, "doc") == 0) {
        if (argc - arg_start < 2) {
            fprintf(stderr, "Usage: nbs-hub doc <list|read|register>\n");
            return 4;
        }
        const char *subcmd = argv[arg_start + 1];

        if (strcmp(subcmd, "list") == 0)
            return hub_doc_list(&ctx);

        if (strcmp(subcmd, "read") == 0) {
            if (argc - arg_start < 3) {
                fprintf(stderr, "Usage: nbs-hub doc read <name>\n");
                return 4;
            }
            return hub_doc_read(&ctx, argv[arg_start + 2]);
        }

        if (strcmp(subcmd, "register") == 0) {
            if (argc - arg_start < 4) {
                fprintf(stderr, "Usage: nbs-hub doc register <name> <path>\n");
                return 4;
            }
            return hub_doc_register(&ctx, argv[arg_start + 2],
                                    argv[arg_start + 3]);
        }

        fprintf(stderr, "Unknown doc subcommand: %s\n", subcmd);
        return 4;
    }

    fprintf(stderr, "Unknown command: %s\n", cmd);
    hub_help();
    return 4;
}
