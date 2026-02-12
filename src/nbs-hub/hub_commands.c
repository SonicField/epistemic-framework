/*
 * hub_commands.c — NBS Hub command implementations
 *
 * Each command:
 *   1. Validates arguments
 *   2. Performs its action
 *   3. Updates state atomically
 *   4. Logs to hub.log and hub.chat
 *   5. Returns exit code
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>
#include <limits.h>

#include "hub_commands.h"
#include "hub_log.h"
#include "chat_file.h"  /* chat_create, chat_send, ASSERT_MSG */

/* ── Helpers ──────────────────────────────────────────────────── */

/* Run an external command and capture stdout. Returns exit code. */
static int run_capture(char *output, size_t output_sz,
                       const char *cmd, char *const argv[])
{
    int pipefd[2];
    if (pipe(pipefd) != 0) return -1;

    pid_t pid = fork();
    if (pid < 0) { close(pipefd[0]); close(pipefd[1]); return -1; }

    if (pid == 0) {
        /* Child */
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execvp(cmd, argv);
        _exit(127);
    }

    /* Parent */
    close(pipefd[1]);
    size_t total = 0;
    ssize_t n;
    while ((n = read(pipefd[0], output + total, output_sz - total - 1)) > 0) {
        total += (size_t)n;
    }
    output[total] = '\0';
    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* Run an external command, passing stdout/stderr through. Returns exit code. */
static int run_passthrough(const char *cmd, char *const argv[])
{
    pid_t pid = fork();
    if (pid < 0) return -1;

    if (pid == 0) {
        execvp(cmd, argv);
        _exit(127);
    }

    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* Check if a file exists and is non-empty */
static int file_nonempty(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return st.st_size > 0;
}

/* Get nbs-worker command name (allow override for testing) */
static const char *get_worker_cmd(void)
{
    const char *cmd = getenv("NBS_WORKER_CMD");
    return cmd ? cmd : "nbs-worker";
}

/* ── hub init ─────────────────────────────────────────────────── */

int hub_init(hub_ctx *ctx, const char *project_dir, const char *goal)
{
    /* Resolve project_dir to absolute path */
    char abs_dir[HUB_MAX_PATH];
    if (project_dir[0] == '/') {
        snprintf(abs_dir, sizeof(abs_dir), "%s", project_dir);
    } else {
        char cwd[HUB_MAX_PATH];
        ASSERT_MSG(getcwd(cwd, sizeof(cwd)) != NULL, "getcwd failed");
        int n = snprintf(abs_dir, sizeof(abs_dir), "%s/%s", cwd, project_dir);
        ASSERT_MSG(n > 0 && n < (int)sizeof(abs_dir), "path overflow");
    }

    /* Check for existing hub */
    int n = snprintf(ctx->hub_dir, sizeof(ctx->hub_dir),
                     "%s/.nbs/hub", abs_dir);
    ASSERT_MSG(n > 0 && n < (int)sizeof(ctx->hub_dir), "path overflow");

    struct stat st;
    if (stat(ctx->hub_dir, &st) == 0) {
        fprintf(stderr, "error: hub already initialised at %s\n", ctx->hub_dir);
        fprintf(stderr, "  Use 'nbs-hub status' to check state.\n");
        return 1;
    }

    /* Create directory structure */
    if (hub_create_dirs(abs_dir) != 0) return -1;

    /* Set up chat path */
    n = snprintf(ctx->chat_path, sizeof(ctx->chat_path),
                 "%s/.nbs/chat/hub.chat", abs_dir);
    ASSERT_MSG(n > 0 && n < (int)sizeof(ctx->chat_path), "path overflow");

    /* Create hub.chat if it doesn't exist */
    if (access(ctx->chat_path, F_OK) != 0) {
        chat_create(ctx->chat_path);
    }

    /* Write manifest */
    hub_manifest *m = &ctx->manifest;
    memset(m, 0, sizeof(*m));
    snprintf(m->project_dir, sizeof(m->project_dir), "%s", abs_dir);
    snprintf(m->terminal_goal, sizeof(m->terminal_goal), "%s", goal);
    n = snprintf(m->workers_dir, sizeof(m->workers_dir),
                 "%s/.nbs/workers", abs_dir);
    ASSERT_MSG(n > 0 && n < (int)sizeof(m->workers_dir), "path overflow");
    snprintf(m->hub_dir, sizeof(m->hub_dir), "%s", ctx->hub_dir);

    if (hub_save_manifest(ctx) != 0) return -1;

    /* Write initial state */
    hub_state *s = &ctx->state;
    memset(s, 0, sizeof(*s));
    snprintf(s->phase_name, sizeof(s->phase_name), "%s", "PLANNING");
    s->stall_threshold_seconds = 1800;

    if (hub_save_state(ctx) != 0) return -1;

    /* Create empty hub.log */
    if (hub_log_open(ctx) != 0) return -1;
    hub_log_write(ctx, "INIT project=%s goal=\"%s\"", abs_dir, goal);
    hub_chat_log(ctx, "init project=%s goal=\"%s\"", abs_dir, goal);
    hub_log_close(ctx);

    printf("Hub initialised.\n");
    printf("  Project: %s\n", abs_dir);
    printf("  Goal: %s\n", goal);
    printf("  Hub dir: %s\n", ctx->hub_dir);
    printf("\nNext steps:\n");
    printf("  nbs-hub doc register <name> <path>  — register project documents\n");
    printf("  nbs-hub spawn <slug> <task>          — spawn a worker\n");

    return 0;
}

/* ── hub status ───────────────────────────────────────────────── */

int hub_status(hub_ctx *ctx)
{
    hub_manifest *m = &ctx->manifest;
    hub_state *s = &ctx->state;

    printf("=== NBS Hub Status ===\n");
    printf("Project:             %s\n", m->project_dir);
    printf("Terminal goal:       %s\n", m->terminal_goal);
    printf("Phase:               %d — %s\n", s->phase, s->phase_name);
    if (s->phase_gate_criteria[0])
        printf("Gate criteria:       %s\n", s->phase_gate_criteria);
    printf("Workers total:       %d\n", s->workers_completed);
    printf("Workers since check: %d\n", s->workers_since_check);
    printf("Audit required:      %s\n", s->audit_required ? "YES" : "no");
    if (s->last_audit_time)
        printf("Last audit:          %s\n", format_time(s->last_audit_time));
    if (s->last_spawn_time)
        printf("Last spawn:          %s\n", format_time(s->last_spawn_time));

    /* Active workers */
    const char *worker_cmd = get_worker_cmd();
    char *argv[] = { (char *)worker_cmd, "list", NULL };
    char output[HUB_MAX_LINE * 10];
    int rc = run_capture(output, sizeof(output), worker_cmd, argv);

    printf("\n=== Active Workers ===\n");
    if (rc == 0 && output[0]) {
        printf("%s", output);
    } else {
        printf("  (none)\n");
    }

    /* Registered documents */
    printf("\n=== Registered Documents ===\n");
    if (m->doc_count == 0) {
        printf("  (none)\n");
    } else {
        for (int i = 0; i < m->doc_count; i++) {
            printf("  %-24s %s\n", m->docs[i].name, m->docs[i].path);
        }
    }

    /* Recent log */
    printf("\n");
    if (hub_log_open(ctx) == 0) {
        hub_log_show(ctx, 10);
        hub_log_close(ctx);
    }

    return 0;
}

/* ── hub spawn ────────────────────────────────────────────────── */

int hub_spawn(hub_ctx *ctx, int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: nbs-hub spawn <slug> <task-description>\n");
        return 4;
    }

    const char *slug = argv[0];
    const char *task = argv[1];

    /* Enforce audit gate */
    if (ctx->state.audit_required) {
        fprintf(stderr, "[HUB-GATE] Audit required before spawning.\n");
        fprintf(stderr, "  Workers since last check: %d\n",
                ctx->state.workers_since_check);
        if (ctx->state.last_audit_time)
            fprintf(stderr, "  Last audit: %s\n",
                    format_time(ctx->state.last_audit_time));
        fprintf(stderr, "  Submit with: nbs-hub audit <file>\n");

        if (hub_log_open(ctx) == 0) {
            hub_log_write(ctx, "SPAWN_REFUSED worker=%s reason=\"audit_required\"",
                          slug);
            hub_chat_log(ctx, "spawn-refused worker=%s reason=audit-overdue "
                         "workers-since-check=%d",
                         slug, ctx->state.workers_since_check);
            hub_log_close(ctx);
        }
        return 3;
    }

    /* Delegate to nbs-worker */
    const char *worker_cmd = get_worker_cmd();
    char *spawn_argv[] = {
        (char *)worker_cmd, "spawn", (char *)slug,
        (char *)ctx->manifest.project_dir, (char *)task, NULL
    };
    char output[HUB_MAX_LINE];
    int rc = run_capture(output, sizeof(output), worker_cmd, spawn_argv);

    if (rc != 0) {
        fprintf(stderr, "error: nbs-worker spawn failed (exit %d)\n", rc);
        return rc;
    }

    /* Strip trailing newline from worker name */
    size_t olen = strlen(output);
    while (olen > 0 && (output[olen - 1] == '\n' || output[olen - 1] == '\r'))
        output[--olen] = '\0';

    /* Update state */
    ctx->state.total_prompts++;
    ctx->state.last_spawn_time = time(NULL);

    if (hub_save_state(ctx) != 0) return -1;

    /* Log */
    if (hub_log_open(ctx) == 0) {
        hub_log_write(ctx, "SPAWN worker=%s task=\"%s\"", output, task);
        hub_chat_log(ctx, "spawn worker=%s task=\"%s\"", output, task);
        hub_log_close(ctx);
    }

    printf("Spawned: %s\n", output);
    return 0;
}

/* ── hub check ────────────────────────────────────────────────── */

int hub_check(hub_ctx *ctx, const char *worker_name)
{
    ASSERT_MSG(worker_name != NULL, "worker_name is NULL");
    (void)ctx;

    const char *worker_cmd = get_worker_cmd();
    char *argv[] = { (char *)worker_cmd, "status", (char *)worker_name, NULL };
    return run_passthrough(worker_cmd, argv);
}

/* ── hub result ───────────────────────────────────────────────── */

int hub_result(hub_ctx *ctx, const char *worker_name)
{
    ASSERT_MSG(worker_name != NULL, "worker_name is NULL");

    const char *worker_cmd = get_worker_cmd();
    char *argv[] = { (char *)worker_cmd, "results", (char *)worker_name, NULL };
    char output[HUB_MAX_LINE * 10];
    int rc = run_capture(output, sizeof(output), worker_cmd, argv);

    if (rc == 0) {
        printf("%s", output);
    }

    /* Update counters */
    ctx->state.workers_completed++;
    ctx->state.workers_since_check++;

    /* Check if audit is now required */
    if (ctx->state.workers_since_check >= 3) {
        ctx->state.audit_required = 1;
        printf("\n[HUB-GATE] Self-check required before next worker spawn.\n");
        printf("  Workers since last check: %d\n",
               ctx->state.workers_since_check);
        printf("  Submit with: nbs-hub audit <file>\n");
    }

    if (hub_save_state(ctx) != 0) return -1;

    /* Log */
    if (hub_log_open(ctx) == 0) {
        hub_log_write(ctx, "RESULT worker=%s status=completed", worker_name);
        hub_log_write(ctx, "COUNTER workers_completed=%d workers_since_check=%d",
                      ctx->state.workers_completed,
                      ctx->state.workers_since_check);
        if (ctx->state.audit_required) {
            hub_log_write(ctx, "AUDIT_REQUIRED reason=\"workers_since_check=%d\"",
                          ctx->state.workers_since_check);
            hub_chat_log(ctx, "audit-required since:%d-workers",
                         ctx->state.workers_since_check);
        }
        hub_log_close(ctx);
    }

    return rc;
}

/* ── hub dismiss ──────────────────────────────────────────────── */

int hub_dismiss(hub_ctx *ctx, const char *worker_name)
{
    ASSERT_MSG(worker_name != NULL, "worker_name is NULL");

    const char *worker_cmd = get_worker_cmd();
    char *argv[] = { (char *)worker_cmd, "dismiss", (char *)worker_name, NULL };
    int rc = run_passthrough(worker_cmd, argv);

    if (rc == 0 && hub_log_open(ctx) == 0) {
        hub_log_write(ctx, "DISMISS worker=%s", worker_name);
        hub_log_close(ctx);
    }

    return rc;
}

/* ── hub list ─────────────────────────────────────────────────── */

int hub_list(hub_ctx *ctx)
{
    (void)ctx;
    const char *worker_cmd = get_worker_cmd();
    char *argv[] = { (char *)worker_cmd, "list", NULL };
    return run_passthrough(worker_cmd, argv);
}

/* ── hub audit ────────────────────────────────────────────────── */

int hub_audit(hub_ctx *ctx, const char *audit_file)
{
    ASSERT_MSG(audit_file != NULL, "audit_file is NULL");

    /* Validate file exists and is non-empty */
    if (!file_nonempty(audit_file)) {
        fprintf(stderr, "error: audit file missing or empty: %s\n", audit_file);
        return 1;
    }

    /* Basic content validation: check for required sections */
    FILE *fp = fopen(audit_file, "r");
    if (!fp) {
        fprintf(stderr, "error: cannot read audit file: %s\n", strerror(errno));
        return 1;
    }

    int has_goal = 0, has_delegate = 0, has_learnings = 0;
    char line[HUB_MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        /* Look for key phrases that indicate substantive self-check content */
        if (strstr(line, "goal") || strstr(line, "Goal") ||
            strstr(line, "terminal") || strstr(line, "Terminal"))
            has_goal = 1;
        if (strstr(line, "delegat") || strstr(line, "Delegat") ||
            strstr(line, "worker") || strstr(line, "Worker"))
            has_delegate = 1;
        if (strstr(line, "learn") || strstr(line, "Learn") ||
            strstr(line, "better") || strstr(line, "Better") ||
            strstr(line, "3W") || strstr(line, "went well"))
            has_learnings = 1;
    }
    fclose(fp);

    if (!has_goal || !has_delegate || !has_learnings) {
        fprintf(stderr, "[HUB-GATE] Audit file appears incomplete.\n");
        fprintf(stderr, "  Required content (at least mention of):\n");
        if (!has_goal)
            fprintf(stderr, "  - Terminal goal alignment\n");
        if (!has_delegate)
            fprintf(stderr, "  - Delegation vs doing tactical work\n");
        if (!has_learnings)
            fprintf(stderr, "  - Learnings / 3Ws\n");
        return 1;
    }

    /* Archive the audit */
    char archive_path[HUB_MAX_PATH];
    int audit_num = ctx->state.workers_completed;  /* rough numbering */
    int n = snprintf(archive_path, sizeof(archive_path),
                     "%s/audits/audit-%03d.md", ctx->hub_dir, audit_num);
    ASSERT_MSG(n > 0 && n < (int)sizeof(archive_path), "path overflow");

    /* Copy file to archive */
    FILE *src = fopen(audit_file, "r");
    FILE *dst = fopen(archive_path, "w");
    if (src && dst) {
        char buf[4096];
        size_t nr;
        while ((nr = fread(buf, 1, sizeof(buf), src)) > 0) {
            fwrite(buf, 1, nr, dst);
        }
    }
    if (src) fclose(src);
    if (dst) fclose(dst);

    /* Reset counters */
    ctx->state.workers_since_check = 0;
    ctx->state.audit_required = 0;
    ctx->state.last_audit_time = time(NULL);

    if (hub_save_state(ctx) != 0) return -1;

    /* Log */
    if (hub_log_open(ctx) == 0) {
        hub_log_write(ctx, "AUDIT file=%s archived=%s", audit_file, archive_path);
        hub_log_write(ctx, "COUNTER workers_since_check=0");
        hub_chat_log(ctx, "audit-accepted file=%s", audit_file);
        hub_log_close(ctx);
    }

    printf("Audit accepted.\n");
    printf("  Archived to: %s\n", archive_path);
    printf("  Workers since check: 0 (reset)\n");
    printf("  You may proceed.\n");

    return 0;
}

/* ── hub gate ─────────────────────────────────────────────────── */

int hub_gate(hub_ctx *ctx, int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: nbs-hub gate <phase-name> <test-results> <audit-file>\n");
        return 4;
    }

    const char *phase_name = argv[0];
    const char *test_file = argv[1];
    const char *audit_file = argv[2];

    /* Verify current phase matches */
    if (strcmp(ctx->state.phase_name, phase_name) != 0) {
        fprintf(stderr, "[HUB-GATE] Phase mismatch.\n");
        fprintf(stderr, "  Current phase: %s\n", ctx->state.phase_name);
        fprintf(stderr, "  Requested gate: %s\n", phase_name);
        fprintf(stderr, "  Cannot skip phases.\n");
        return 1;
    }

    /* Validate test results */
    if (!file_nonempty(test_file)) {
        fprintf(stderr, "[HUB-GATE] Test results file missing or empty: %s\n",
                test_file);
        return 1;
    }

    /* Validate audit */
    if (!file_nonempty(audit_file)) {
        fprintf(stderr, "[HUB-GATE] Audit file missing or empty: %s\n",
                audit_file);
        return 1;
    }

    /* Archive gate submission */
    char gate_path[HUB_MAX_PATH];
    int n = snprintf(gate_path, sizeof(gate_path),
                     "%s/gates/phase-%d-gate.md", ctx->hub_dir, ctx->state.phase);
    ASSERT_MSG(n > 0 && n < (int)sizeof(gate_path), "path overflow");

    FILE *gf = fopen(gate_path, "w");
    if (gf) {
        fprintf(gf, "# Phase %d Gate: %s\n\n", ctx->state.phase, phase_name);
        fprintf(gf, "Passed: %s\n", format_time(time(NULL)));
        fprintf(gf, "Test results: %s\n", test_file);
        fprintf(gf, "Audit file: %s\n", audit_file);
        fclose(gf);
    }

    /* Advance phase */
    int old_phase = ctx->state.phase;
    ctx->state.phase++;
    memset(ctx->state.phase_name, 0, HUB_MAX_NAME);
    memset(ctx->state.phase_gate_criteria, 0, HUB_MAX_GATE_CRITERIA);
    /* Phase name/criteria will be set by supervisor after gate passage */

    /* Reset audit counters */
    ctx->state.workers_since_check = 0;
    ctx->state.audit_required = 0;
    ctx->state.last_audit_time = time(NULL);

    if (hub_save_state(ctx) != 0) return -1;

    /* Log */
    if (hub_log_open(ctx) == 0) {
        hub_log_write(ctx, "GATE_PASSED phase=%d name=\"%s\"",
                      old_phase, phase_name);
        hub_chat_log(ctx, "gate-passed phase=%d name=\"%s\"",
                     old_phase, phase_name);
        hub_log_close(ctx);
    }

    printf("Phase %d (%s) complete.\n", old_phase, phase_name);
    printf("  Now in phase %d.\n", ctx->state.phase);
    printf("  Set phase name: update state file or use nbs-hub phase-set (future)\n");

    return 0;
}

/* ── hub phase ────────────────────────────────────────────────── */

int hub_phase(hub_ctx *ctx)
{
    printf("Phase:    %d — %s\n", ctx->state.phase, ctx->state.phase_name);
    if (ctx->state.phase_gate_criteria[0])
        printf("Gate:     %s\n", ctx->state.phase_gate_criteria);
    printf("Workers:  %d completed, %d since last check\n",
           ctx->state.workers_completed, ctx->state.workers_since_check);
    printf("Audit:    %s\n", ctx->state.audit_required ? "REQUIRED" : "not required");
    return 0;
}

/* ── hub doc list ─────────────────────────────────────────────── */

int hub_doc_list(hub_ctx *ctx)
{
    hub_manifest *m = &ctx->manifest;

    if (m->doc_count == 0) {
        printf("No documents registered.\n");
        printf("  Register with: nbs-hub doc register <name> <path>\n");
        return 0;
    }

    printf("=== Registered Documents ===\n");
    for (int i = 0; i < m->doc_count; i++) {
        /* Check if file exists */
        const char *status = "";
        if (access(m->docs[i].path, F_OK) != 0) {
            status = "  [MISSING]";
        }
        printf("  %-24s %s%s\n", m->docs[i].name, m->docs[i].path, status);
    }

    return 0;
}

/* ── hub doc read ─────────────────────────────────────────────── */

int hub_doc_read(hub_ctx *ctx, const char *name)
{
    ASSERT_MSG(name != NULL, "doc name is NULL");

    hub_manifest *m = &ctx->manifest;

    /* Find document */
    for (int i = 0; i < m->doc_count; i++) {
        if (strcmp(m->docs[i].name, name) == 0) {
            /* Check file exists */
            if (access(m->docs[i].path, F_OK) != 0) {
                printf("[HUB-WARNING] Registered path does not exist: %s\n",
                       m->docs[i].path);
                printf("  The file may have been moved or deleted.\n");
                printf("  To update: nbs-hub doc register %s <new-path>\n", name);
                return 1;
            }

            /* Output file content */
            FILE *fp = fopen(m->docs[i].path, "r");
            if (!fp) {
                fprintf(stderr, "error: cannot read %s: %s\n",
                        m->docs[i].path, strerror(errno));
                return 1;
            }
            char buf[4096];
            size_t n;
            while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
                fwrite(buf, 1, n, stdout);
            }
            fclose(fp);
            return 0;
        }
    }

    /* Not found */
    printf("[HUB-QUESTION] I do not have a record for \"%s\".\n", name);
    printf("  To register: nbs-hub doc register %s <path>\n", name);
    return 2;
}

/* ── hub doc register ─────────────────────────────────────────── */

int hub_doc_register(hub_ctx *ctx, const char *name, const char *path)
{
    ASSERT_MSG(name != NULL, "doc name is NULL");
    ASSERT_MSG(path != NULL, "doc path is NULL");

    hub_manifest *m = &ctx->manifest;

    /* Resolve to absolute path */
    char abs_path[HUB_MAX_PATH];
    if (path[0] == '/') {
        snprintf(abs_path, sizeof(abs_path), "%s", path);
    } else {
        char *r = realpath(path, abs_path);
        if (!r) {
            /* File might not exist yet — resolve directory part */
            char cwd[HUB_MAX_PATH];
            ASSERT_MSG(getcwd(cwd, sizeof(cwd)) != NULL, "getcwd failed");
            int n = snprintf(abs_path, sizeof(abs_path), "%s/%s", cwd, path);
            ASSERT_MSG(n > 0 && n < (int)sizeof(abs_path), "path overflow");
        }
    }

    /* Check if already registered — update path if so */
    for (int i = 0; i < m->doc_count; i++) {
        if (strcmp(m->docs[i].name, name) == 0) {
            snprintf(m->docs[i].path, sizeof(m->docs[i].path), "%s", abs_path);
            if (hub_save_manifest(ctx) != 0) return -1;

            if (hub_log_open(ctx) == 0) {
                hub_log_write(ctx, "DOC_UPDATE name=%s path=%s", name, abs_path);
                hub_log_close(ctx);
            }

            printf("Updated: %s → %s\n", name, abs_path);
            return 0;
        }
    }

    /* New registration */
    ASSERT_MSG(m->doc_count < HUB_MAX_DOCS,
               "too many docs: %d >= %d", m->doc_count, HUB_MAX_DOCS);

    hub_doc_entry *e = &m->docs[m->doc_count];
    snprintf(e->name, sizeof(e->name), "%s", name);
    snprintf(e->path, sizeof(e->path), "%s", abs_path);
    m->doc_count++;

    if (hub_save_manifest(ctx) != 0) return -1;

    if (hub_log_open(ctx) == 0) {
        hub_log_write(ctx, "DOC_REGISTER name=%s path=%s", name, abs_path);
        hub_log_close(ctx);
    }

    printf("Registered: %s → %s\n", name, abs_path);
    return 0;
}

/* ── hub decision ─────────────────────────────────────────────── */

int hub_decision(hub_ctx *ctx, const char *text)
{
    ASSERT_MSG(text != NULL, "decision text is NULL");

    /* Append to decisions.log if registered, else to hub.log */
    if (hub_log_open(ctx) == 0) {
        hub_log_write(ctx, "DECISION %s", text);
        hub_chat_log(ctx, "decision \"%s\"", text);
        hub_log_close(ctx);
    }

    printf("Decision recorded.\n");
    return 0;
}

/* ── hub help ─────────────────────────────────────────────────── */

void hub_help(void)
{
    printf("nbs-hub — NBS Teams process enforcement hub\n\n");
    printf("Usage: nbs-hub <command> [args...]\n\n");
    printf("Commands:\n");
    printf("  init <dir> <goal>              Initialise hub for a project\n");
    printf("  status                         Full project state dump\n");
    printf("  spawn <slug> <task>            Spawn a worker (enforces audit gate)\n");
    printf("  check <worker>                 Check worker status\n");
    printf("  result <worker>                Read worker result (updates counters)\n");
    printf("  dismiss <worker>               Dismiss a worker\n");
    printf("  list                           List all workers\n");
    printf("  audit <file>                   Submit NBS self-check audit\n");
    printf("  gate <phase> <tests> <audit>   Submit phase gate\n");
    printf("  phase                          Show current phase\n");
    printf("  doc list                       List registered documents\n");
    printf("  doc read <name>                Output document content\n");
    printf("  doc register <name> <path>     Register a document\n");
    printf("  decision <text>                Record a decision\n");
    printf("  log [n]                        Show last n log entries\n");
    printf("  help                           Show this help\n");
    printf("\nExit codes:\n");
    printf("  0  Success\n");
    printf("  1  Validation error (gate refused, file missing)\n");
    printf("  2  Hub not found / document not registered\n");
    printf("  3  Spawn refused (audit required)\n");
    printf("  4  Usage error\n");
}
