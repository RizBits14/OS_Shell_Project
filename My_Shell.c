#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_INPUT 256
#define MAX_ARGS 128

int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];

    while (1) {
        printf("sh> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) break;

        int len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') input[len - 1] = '\0';

        if (strcmp(input, "exit") == 0) break;

        // Handle piping
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
            char *cmd = commands[i];
            char *in_file = NULL, *out_file = NULL;
            int append = 0;

            // Handle redirection
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

            if (in_file) while (*in_file == ' ') in_file++;
            if (out_file) while (*out_file == ' ') out_file++;

            // Tokenize arguments
            int j = 0;
            char *arg = strtok(cmd, " ");
            while (arg && j < MAX_ARGS - 1) {
                args[j++] = arg;
                arg = strtok(NULL, " ");
            }
            args[j] = NULL;

            // Handle built-in 'cd' manually (only when no pipe)
            if (num_cmds == 1 && args[0] && strcmp(args[0], "cd") == 0) {
                if (args[1]) {
                    if (chdir(args[1]) != 0) {
                        perror("cd failed");
                    }
                } else {
                    fprintf(stderr, "cd: missing argument\n");
                }
                continue;
            }

            pid_t pid = fork();
            if (pid == 0) {
                // Piping setup
                if (i > 0) dup2(pipes[(i - 1) * 2], STDIN_FILENO);
                if (i < num_cmds - 1) dup2(pipes[i * 2 + 1], STDOUT_FILENO);

                // Close all pipe fds
                for (int k = 0; k < 2 * (num_cmds - 1); k++) {
                    close(pipes[k]);
                }

                // Input redirection
                if (in_file) {
                    int fd_in = open(in_file, O_RDONLY);
                    if (fd_in < 0) {
                        perror("Input file error");
                        exit(1);
                    }
                    dup2(fd_in, STDIN_FILENO);
                    close(fd_in);
                }

                // Output redirection
                if (out_file) {
                    int fd_out = open(out_file, O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC), 0644);
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

        // Close all pipes in parent
        for (int i = 0; i < 2 * (num_cmds - 1); i++) {
            close(pipes[i]);
        }

        // Wait for children
        for (int i = 0; i < num_cmds; i++) {
            wait(NULL);
        }
    }

    return 0;
}
