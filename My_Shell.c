#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_INPUT 256
#define MAX_ARGS 128
#define MAX_HISTORY 100

char *history[MAX_HISTORY];
int history_count = 0;

void sigint_handler(int signo) {
    printf("\n[Shell] CTRL+C ignored. Type 'exit' to quit.\nsh> ");
    fflush(stdout);
}

char *trim(char *str) {
    while (*str == ' ' || *str == '\t') str++;
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t')) *end-- = '\0';
    return str;
}

void add_to_history(const char *cmd) {
    if (history_count < MAX_HISTORY) {
        history[history_count++] = strdup(cmd);
    }
}

void show_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d: %s\n", i + 1, history[i]);
    }
}

void execute_pipeline(char *line) {
    char *commands[MAX_ARGS];
    int num_cmds = 0;
    char *pipe_token = strtok(line, "|");
    while (pipe_token && num_cmds < MAX_ARGS) {
        commands[num_cmds++] = pipe_token;
        pipe_token = strtok(NULL, "|");
    }

    int pipes[2 * (num_cmds - 1)];
    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipes + i * 2) < 0) {
            perror("Pipe creation failed");
            exit(1);
        }
    }

    for (int i = 0; i < num_cmds; i++) {
        char *cmd = commands[i];
        char *args[MAX_ARGS];
        char *in_file = NULL, *out_file = NULL;
        int append = 0;

        char *out = strstr(cmd, ">>");
        if (out) {
            append = 1;
            *out = '\0';
            out_file = out + 2;
        } else if ((out = strchr(cmd, '>'))) {
            *out = '\0';
            out_file = out + 1;
        }

        char *in = strchr(cmd, '<');
        if (in) {
            *in = '\0';
            in_file = in + 1;
        }

        if (in_file) in_file = trim(in_file);
        if (out_file) out_file = trim(out_file);

        int j = 0;
        char *arg = strtok(cmd, " ");
        while (arg && j < MAX_ARGS - 1) {
            args[j++] = arg;
            arg = strtok(NULL, " ");
        }
        args[j] = NULL;

        pid_t pid = fork();
        if (pid == 0) {
            if (i > 0) dup2(pipes[(i - 1) * 2], STDIN_FILENO);
            if (i < num_cmds - 1) dup2(pipes[i * 2 + 1], STDOUT_FILENO);
            for (int k = 0; k < 2 * (num_cmds - 1); k++) close(pipes[k]);

            if (in_file) {
                int fd_in = open(in_file, O_RDONLY);
                if (fd_in < 0) {
                    perror("Input file error");
                    exit(1);
                }
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }

            if (out_file) {
                int fd_out = append
                    ? open(out_file, O_WRONLY | O_CREAT | O_APPEND, 0644)
                    : open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd_out < 0) {
                    perror("Output file error");
                    exit(1);
                }
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }

            execvp(args[0], args);
            perror("Command failed");
            exit(1);
        }
    }

    for (int i = 0; i < 2 * (num_cmds - 1); i++) close(pipes[i]);
    for (int i = 0; i < num_cmds; i++) wait(NULL);
}

void execute_input(char *input) {
    char *curr = input;
    int execute_next = 1;

    while (curr && *curr) {
        char *next_cmd = NULL;
        char *semicolon = strstr(curr, ";");
        char *and_and = strstr(curr, "&&");

        if (and_and && (!semicolon || and_and < semicolon)) {
            *and_and = '\0';
            next_cmd = and_and + 2;
        } else if (semicolon) {
            *semicolon = '\0';
            next_cmd = semicolon + 1;
            execute_next = 1;
        }

        char *cmd = trim(curr);
        if (strlen(cmd) == 0) {
            curr = next_cmd;
            continue;
        }

        if (strcmp(cmd, "history") == 0 && execute_next) {
            show_history();
        } else if (strncmp(cmd, "cd ", 3) == 0 && execute_next) {
            char *path = trim(cmd + 3);
            if (chdir(path) != 0) {
                perror("cd failed");
            }
        } else if (strcmp(cmd, "cd") == 0 && execute_next) {
            const char *home = getenv("HOME");
            if (home) chdir(home);
        } else if (execute_next) {
            pid_t pid = fork();
            if (pid == 0) {
                execute_pipeline(cmd);
                exit(0);
            } else {
                int status;
                waitpid(pid, &status, 0);
                execute_next = WIFEXITED(status) && WEXITSTATUS(status) == 0;
            }
        }

        curr = next_cmd;
    }
}

int main() {
    char input[MAX_INPUT];

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    while (1) {
        printf("sh> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';
        char *clean_input = trim(input);
        if (strlen(clean_input) == 0) continue;

        if (strcmp(clean_input, "exit") == 0) break;

        add_to_history(clean_input);
        execute_input(clean_input);
    }

    for (int i = 0; i < history_count; i++) {
        free(history[i]);
    }

    return 0;
}
