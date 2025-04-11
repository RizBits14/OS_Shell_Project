#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h> // for open()

#define MAX_INPUT 256
#define MAX_ARGS 128

// 🔹 Feature 1 + 2 + 3
int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];

    while (1) {
        printf("sh> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        int len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        if (strcmp(input, "exit") == 0) {
            break;
        }

        // 🔹 Feature 3: Handle piping
        char *commands[MAX_ARGS];
        int num_cmds = 0;
        char *pipe_token = strtok(input, "|");
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
            // Handle redirection inside each command
            char *cmd = commands[i];
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

            // Trim spaces from filenames
            if (in_file) while (*in_file == ' ') in_file++;
            if (out_file) while (*out_file == ' ') out_file++;

            // Tokenize command into arguments
            int j = 0;
            char *arg = strtok(cmd, " ");
            while (arg && j < MAX_ARGS - 1) {
                args[j++] = arg;
                arg = strtok(NULL, " ");
            }
            args[j] = NULL;

            pid_t pid = fork();

            if (pid == 0) {
                // 🔹 Feature 3: Setup input pipe
                if (i > 0) {
                    dup2(pipes[(i - 1) * 2], STDIN_FILENO);
                }

                // 🔹 Feature 3: Setup output pipe
                if (i < num_cmds - 1) {
                    dup2(pipes[i * 2 + 1], STDOUT_FILENO);
                }

                // Close all pipe fds
                for (int k = 0; k < 2 * (num_cmds - 1); k++) {
                    close(pipes[k]);
                }

                // 🔹 Feature 2: Handle input redirection
                if (in_file) {
                    int fd_in = open(in_file, O_RDONLY);
                    if (fd_in < 0) {
                        perror("Input file error");
                        exit(1);
                    }
                    dup2(fd_in, STDIN_FILENO);
                    close(fd_in);
                }

                // 🔹 Feature 2: Handle output redirection
                if (out_file) {
                    int fd_out;
                    if (append) {
                        fd_out = open(out_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    } else {
                        fd_out = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    }
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

        // Close all pipe fds in parent
        for (int i = 0; i < 2 * (num_cmds - 1); i++) {
            close(pipes[i]);
        }

        // Wait for all child processes
        for (int i = 0; i < num_cmds; i++) {
            wait(NULL);
        }
    }

    return 0;
}
