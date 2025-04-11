#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 256
#define MAX_PARTS 128

int main() {
    char line[MAX_LINE];
    char *parts[MAX_PARTS];

    while (1) {
        printf("sh> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }

        int len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        if (strcmp(line, "exit") == 0) {
            break;
        }

        // Handle pipe: Split input by '|'
        char *commands[MAX_PARTS];
        int num_commands = 0;

        char *cmd = strtok(line, "|");
        while (cmd != NULL && num_commands < MAX_PARTS) {
            commands[num_commands++] = cmd;
            cmd = strtok(NULL, "|");
        }

        int pipe_fds[2 * (num_commands - 1)];
        for (int i = 0; i < num_commands - 1; i++) {
            if (pipe(pipe_fds + 2 * i) == -1) {
                perror("pipe failed");
                exit(1);
            }
        }

        // Process each command in the pipeline
        for (int i = 0; i < num_commands; i++) {
            // Tokenize the command into parts (command and arguments)
            int j = 0;
            char *token = strtok(commands[i], " ");
            while (token && j < MAX_PARTS - 1) {
                parts[j++] = token;
                token = strtok(NULL, " ");
            }
            parts[j] = NULL;

            pid_t pid = fork();

            if (pid == 0) {
                // If it's not the first command, get input from the previous pipe
                if (i > 0) {
                    if (dup2(pipe_fds[2 * (i - 1)], STDIN_FILENO) == -1) {
                        perror("dup2 input failed");
                        exit(1);
                    }
                }

                // If it's not the last command, send output to the next pipe
                if (i < num_commands - 1) {
                    if (dup2(pipe_fds[2 * i + 1], STDOUT_FILENO) == -1) {
                        perror("dup2 output failed");
                        exit(1);
                    }
                }

                // Close all pipe fds
                for (int j = 0; j < 2 * (num_commands - 1); j++) {
                    close(pipe_fds[j]);
                }

                execvp(parts[0], parts);
                perror("Command not found");
                exit(1);
            }
        }

        // Close all pipe fds in the parent
        for (int i = 0; i < 2 * (num_commands - 1); i++) {
            close(pipe_fds[i]);
        }

        // Wait for all child processes to finish
        for (int i = 0; i < num_commands; i++) {
            wait(NULL);
        }
    }

    return 0;
}

