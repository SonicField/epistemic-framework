/*
 * terminal.c — Interactive terminal client for nbs-chat
 *
 * Usage: nbs-chat-terminal <file> <handle>
 *
 * Controls:
 *   Type a message and press Enter to add a line.
 *   Press Enter on an empty line to send the message.
 *   Type /edit on an empty line to compose in $EDITOR.
 *   Type /help for all commands.
 *   Type /exit or Ctrl-C to exit.
 *
 * New messages from others are displayed each time you press Enter.
 *
 * Exit codes:
 *   0 - Clean exit
 *   1 - General error
 *   2 - Chat file not found
 *   4 - Invalid arguments
 */

#include "chat_file.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

/* --- ANSI colour palette --- */

static const char *COLOURS[] = {
    "38;5;39",   /* Blue */
    "38;5;208",  /* Orange */
    "38;5;41",   /* Green */
    "38;5;213",  /* Pink */
    "38;5;226",  /* Yellow */
    "38;5;87",   /* Cyan */
    "38;5;196",  /* Red */
    "38;5;147",  /* Lavender */
};
#define NUM_COLOURS 8

#define BOLD  "\033[1m"
#define DIM   "\033[2m"
#define RESET "\033[0m"

/* Handle-to-colour mapping */
typedef struct {
    char handle[MAX_HANDLE_LEN];
    int colour_index;
} handle_colour_t;

static handle_colour_t handle_colours[MAX_PARTICIPANTS];
static int handle_colour_count = 0;
static int next_colour = 0;

static const char *get_colour(const char *handle) {
    /* Precondition */
    ASSERT_MSG(handle != NULL, "get_colour: handle is NULL");

    for (int i = 0; i < handle_colour_count; i++) {
        if (strcmp(handle_colours[i].handle, handle) == 0) {
            return COLOURS[handle_colours[i].colour_index];
        }
    }
    if (handle_colour_count < MAX_PARTICIPANTS) {
        snprintf(handle_colours[handle_colour_count].handle, MAX_HANDLE_LEN,
                 "%s", handle);
        handle_colours[handle_colour_count].colour_index = next_colour;
        handle_colour_count++;

        /* Invariant: colour count within bounds */
        ASSERT_MSG(handle_colour_count <= MAX_PARTICIPANTS,
                   "get_colour: handle_colour_count %d exceeds MAX_PARTICIPANTS %d",
                   handle_colour_count, MAX_PARTICIPANTS);

        int idx = next_colour;
        next_colour = (next_colour + 1) % NUM_COLOURS;
        return COLOURS[idx];
    }
    return COLOURS[0];
}

/* --- Global state --- */

static const char *g_chat_file = NULL;
static const char *g_handle = NULL;
static int g_msg_count = 0;
static volatile sig_atomic_t g_quit = 0;

/* --- Display functions --- */

static void format_message(const char *handle, const char *content,
                           const char *my_handle) {
    /* Preconditions */
    ASSERT_MSG(handle != NULL, "format_message: handle is NULL");
    ASSERT_MSG(content != NULL, "format_message: content is NULL");
    ASSERT_MSG(my_handle != NULL, "format_message: my_handle is NULL");

    const char *colour = get_colour(handle);
    if (strcmp(handle, my_handle) == 0) {
        /* Own messages slightly dimmer */
        printf("  %s\033[%sm%s%s%s: %s%s\n",
               DIM, colour, handle, RESET, DIM, content, RESET);
    } else {
        printf("  \033[%sm%s%s%s: %s\n",
               colour, BOLD, handle, RESET, content);
    }
}

static void print_prompt(const char *handle) {
    printf("%s%s>%s ", BOLD, handle, RESET);
    fflush(stdout);
}

static void print_continuation(void) {
    printf("%s...%s ", DIM, RESET);
    fflush(stdout);
}

static void print_help(void) {
    printf("\n");
    printf("%sCommands:%s\n", BOLD, RESET);
    printf("  %s/edit%s   Open $EDITOR to compose a message\n", DIM, RESET);
    printf("  %s/help%s   Show this help\n", DIM, RESET);
    printf("  %s/exit%s   Leave the chat\n", DIM, RESET);
    printf("\n");
    printf("%sInput:%s\n", BOLD, RESET);
    printf("  %sEnter%s        Add a line (continuation prompt: ...)\n",
           DIM, RESET);
    printf("  %sBlank line%s   Send the message\n", DIM, RESET);
    printf("  %sCtrl-C%s       Exit\n", DIM, RESET);
    printf("\n");
}

/* --- Message checking --- */

static void check_new_messages(void) {
    /* Preconditions: global state must be initialised */
    ASSERT_MSG(g_chat_file != NULL,
               "check_new_messages: called before g_chat_file initialised");
    ASSERT_MSG(g_handle != NULL,
               "check_new_messages: called before g_handle initialised");
    /* Invariant: message count must be non-negative */
    ASSERT_MSG(g_msg_count >= 0,
               "check_new_messages: g_msg_count is negative: %d", g_msg_count);

    chat_state_t state;
    if (chat_read(g_chat_file, &state) < 0) return;

    if (state.message_count > g_msg_count) {
        int new_start = g_msg_count;
        g_msg_count = state.message_count;

        for (int i = new_start; i < state.message_count; i++) {
            /* Skip own messages — already displayed when sent */
            if (strcmp(state.messages[i].handle, g_handle) == 0) continue;
            format_message(state.messages[i].handle,
                          state.messages[i].content, g_handle);
        }
    }

    chat_state_free(&state);
}

/* --- Editor mode --- */

static char *open_editor(void) {
    const char *editor = getenv("EDITOR");
    if (!editor) editor = "vim";

    /* Create temp file */
    char tmppath[] = "/tmp/nbs-chat-edit.XXXXXX";
    int fd = mkstemp(tmppath);
    if (fd < 0) return NULL;
    close(fd);

    /* Fork and exec editor */
    pid_t pid = fork();
    if (pid < 0) {
        unlink(tmppath);
        return NULL;
    }

    if (pid == 0) {
        /* Child: run editor with /dev/tty */
        int tty = open("/dev/tty", O_RDONLY);
        if (tty >= 0) {
            dup2(tty, STDIN_FILENO);
            close(tty);
        }
        execlp(editor, editor, tmppath, (char *)NULL);
        _exit(127);
    }

    /* Parent: wait for editor */
    int wstatus;
    waitpid(pid, &wstatus, 0);

    if (!WIFEXITED(wstatus) || WEXITSTATUS(wstatus) != 0) {
        unlink(tmppath);
        return NULL;
    }

    /* Read result */
    FILE *f = fopen(tmppath, "r");
    if (!f) {
        unlink(tmppath);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (len <= 0) {
        fclose(f);
        unlink(tmppath);
        return NULL;
    }

    char *content = malloc(len + 1);
    if (!content) {
        fclose(f);
        unlink(tmppath);
        return NULL;
    }

    size_t nread = fread(content, 1, len, f);
    content[nread] = '\0';
    fclose(f);
    unlink(tmppath);

    /* Trim trailing newlines */
    while (nread > 0 && (content[nread - 1] == '\n' || content[nread - 1] == '\r')) {
        content[--nread] = '\0';
    }

    if (nread == 0) {
        free(content);
        return NULL;
    }

    return content;
}

/* --- Signal handling --- */

static void handle_signal(int sig) {
    (void)sig;
    g_quit = 1;
}

/* --- Line reading with simple line editing --- */

/*
 * Read a line from stdin with basic editing support.
 * Returns the line (caller frees), or NULL on EOF/error.
 * Handles backspace and basic terminal input.
 */
static char *read_line(void) {
    char *buf = malloc(4096);
    if (!buf) return NULL;
    size_t cap = 4096;
    size_t len = 0;

    while (!g_quit) {
        char c;
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n <= 0) {
            if (n == 0 || (errno != EINTR && errno != EAGAIN)) {
                /* EOF */
                if (len == 0) {
                    free(buf);
                    return NULL;
                }
                break;
            }
            continue;
        }

        if (c == '\n' || c == '\r') {
            printf("\n");
            break;
        }

        if (c == 4) { /* Ctrl-D */
            if (len == 0) {
                free(buf);
                return NULL;
            }
            break;
        }

        if (c == 3) { /* Ctrl-C */
            free(buf);
            g_quit = 1;
            return NULL;
        }

        if (c == 127 || c == 8) { /* Backspace / DEL */
            if (len > 0) {
                len--;
                printf("\b \b");
                fflush(stdout);
            }
            continue;
        }

        /* Ignore other control chars except tab */
        if (c < 32 && c != '\t') continue;

        /* Grow buffer if needed */
        if (len + 1 >= cap) {
            cap *= 2;
            char *newbuf = realloc(buf, cap);
            if (!newbuf) {
                free(buf);
                return NULL;
            }
            buf = newbuf;
        }

        buf[len++] = c;
        /* Echo the character */
        write(STDOUT_FILENO, &c, 1);
    }

    buf[len] = '\0';

    /* Postcondition: returned buffer is null-terminated */
    ASSERT_MSG(buf[len] == '\0',
               "read_line: returned buffer not null-terminated at position %zu", len);

    return buf;
}

/* --- Main --- */

static void print_usage(void) {
    printf("nbs-chat-terminal: Interactive terminal client for nbs-chat\n\n");
    printf("Usage:\n");
    printf("  nbs-chat-terminal <file> <handle>\n\n");
    printf("  <file>    Path to chat file (must exist)\n");
    printf("  <handle>  Your display name in the chat\n\n");
    printf("Controls:\n");
    printf("  Type a message and press Enter to add a line.\n");
    printf("  Press Enter on an empty line to send the message.\n");
    printf("  Type /edit on an empty line to compose in $EDITOR.\n");
    printf("  Type /help for all commands.\n");
    printf("  Type /exit or Ctrl-C to exit.\n\n");
    printf("New messages from others are displayed each time you press Enter.\n");
}

int main(int argc, char **argv) {
    if (argc < 3) {
        print_usage();
        return 4;
    }

    g_chat_file = argv[1];
    g_handle = argv[2];

    /* Preconditions: args validated from argv */
    ASSERT_MSG(g_chat_file != NULL, "main: chat_file path is NULL");
    ASSERT_MSG(g_handle != NULL, "main: handle is NULL");

    /* Check file exists */
    struct stat st;
    if (stat(g_chat_file, &st) != 0) {
        fprintf(stderr, "Error: Chat file not found: %s\n", g_chat_file);
        fprintf(stderr, "Create it first: nbs-chat create %s\n", g_chat_file);
        return 2;
    }

    /* Set up signal handlers */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* Put terminal in raw-ish mode (disable echo and canonical mode,
     * but keep signal generation for Ctrl-C) */
    struct termios orig_termios, raw;
    if (tcgetattr(STDIN_FILENO, &orig_termios) == 0) {
        raw = orig_termios;
        raw.c_lflag &= ~(ECHO | ICANON);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }

    /* Show existing messages */
    chat_state_t state;
    if (chat_read(g_chat_file, &state) == 0) {
        for (int i = 0; i < state.message_count; i++) {
            format_message(state.messages[i].handle,
                          state.messages[i].content, g_handle);
        }
        g_msg_count = state.message_count;
        if (state.message_count > 0) printf("\n");
        chat_state_free(&state);
    }

    /* Input buffer for multi-line messages */
    char *input_buffer = NULL;
    size_t input_len = 0;
    size_t input_cap = 0;

    while (!g_quit) {
        /* Check for new messages before showing prompt */
        check_new_messages();

        if (input_len == 0) {
            print_prompt(g_handle);
        } else {
            print_continuation();
        }

        char *line = read_line();

        if (!line) {
            /* EOF or Ctrl-C — send any remaining buffer */
            if (input_buffer && input_len > 0) {
                chat_send(g_chat_file, g_handle, input_buffer);
            }
            break;
        }

        /* /edit command (only on first line of buffer) */
        if (input_len == 0 && strcmp(line, "/edit") == 0) {
            free(line);
            /* Restore terminal for editor */
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
            char *msg = open_editor();
            /* Back to raw mode */
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
            if (msg) {
                if (chat_send(g_chat_file, g_handle, msg) == 0) {
                    format_message(g_handle, msg, g_handle);
                    g_msg_count++;
                } else {
                    printf("  %s(send failed)%s\n", DIM, RESET);
                }
                free(msg);
            } else {
                printf("  %s(empty — not sent)%s\n", DIM, RESET);
            }
            continue;
        }

        /* /exit command */
        if (input_len == 0 && strcmp(line, "/exit") == 0) {
            free(line);
            break;
        }

        /* /help command */
        if (input_len == 0 && strcmp(line, "/help") == 0) {
            free(line);
            print_help();
            continue;
        }

        /* Empty line on empty buffer — ignore */
        if (input_len == 0 && line[0] == '\0') {
            free(line);
            continue;
        }

        /* Empty line with content in buffer — send */
        if (input_len > 0 && line[0] == '\0') {
            free(line);
            if (chat_send(g_chat_file, g_handle, input_buffer) == 0) {
                format_message(g_handle, input_buffer, g_handle);
                g_msg_count++;
            } else {
                printf("  %s(send failed)%s\n", DIM, RESET);
            }
            free(input_buffer);
            input_buffer = NULL;
            input_len = 0;
            input_cap = 0;
            continue;
        }

        /* Non-empty line — append to buffer */
        size_t line_len = strlen(line);
        size_t needed = input_len + line_len + 2; /* +1 for \n, +1 for \0 */
        if (needed > input_cap) {
            input_cap = needed * 2;
            char *newbuf = realloc(input_buffer, input_cap);
            if (!newbuf) {
                free(line);
                break;
            }
            input_buffer = newbuf;
        }
        if (input_len > 0) {
            input_buffer[input_len++] = '\n';
        }
        memcpy(input_buffer + input_len, line, line_len);
        input_len += line_len;
        input_buffer[input_len] = '\0';

        /* Invariant: buffer is null-terminated and within capacity */
        ASSERT_MSG(input_buffer[input_len] == '\0',
                   "input_buffer not null-terminated at position %zu", input_len);
        ASSERT_MSG(input_len < input_cap,
                   "input_len %zu >= input_cap %zu", input_len, input_cap);

        free(line);
    }

    /* Cleanup */
    free(input_buffer);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf("\n%sLeft chat.%s\n", DIM, RESET);

    return 0;
}
