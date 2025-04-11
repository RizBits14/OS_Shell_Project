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
    char *commands[MAX_PARTS];
    char *parts[MAX_PARTS];
    int num_commands;
    int pipe_fds[2 * (MAX_PARTS - 1)]; // Increased size to be safe

    printf("Enter a command with pipes (or 'exit'):\n");
    while (fgets(line, sizeof(line), stdin) != NULL) {
        int len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        if (strcmp(line, "exit") == 0) {
            break;
        }

        num_commands = 0;
        char *line_copy = strdup(line); // Important: strtok modifies the original string
        char *cmd = strtok(line_copy, "|");
        while (cmd != NULL && num_commands < MAX_PARTS) {
            commands[num_commands++] = strdup(cmd); // Duplicate each command string
            cmd = strtok(NULL, "|");
        }
        free(line_copy);

        if (num_commands > 1) {
            // Create pipes
            for (int i = 0; i < num_commands - 1; i++) {
                if (pipe(pipe_fds + 2 * i) == -1) {
                    perror("pipe failed");
                    for (int k = 0; k < num_commands; k++) free(commands[k]);
                    continue; // Or exit, depending on desired behavior
                }
            }

            // Process each command in the pipeline
            for (int i = 0; i < num_commands; i++) {
                int j = 0;
                char *command_copy = strdup(commands[i]);
                char *token = strtok(command_copy, " ");
                while (token && j < MAX_PARTS - 1) {
                    parts[j++] = token;
                    token = strtok(NULL, " ");
                }
                parts[j] = NULL;
                free(command_copy);

                pid_t pid = fork();

                if (pid == 0) {
                    if (i > 0) {
                        if (dup2(pipe_fds[2 * (i - 1)], STDIN_FILENO) == -1) {
                            perror("dup2 input failed");
                            exit(1);
                        }
                    }
                    if (i < num_commands - 1) {
                        if (dup2(pipe_fds[2 * i + 1], STDOUT_FILENO) == -1) {
                            perror("dup2 output failed");
                            exit(1);
                        }
                    }
                    for (int k = 0; k < 2 * (num_commands - 1); k++) {
                        close(pipe_fds[k]);
                    }
                    execvp(parts[0], parts);
                    perror("Command not found");
                    exit(1);
                } else if (pid < 0) {
                    perror("fork failed");
                }
            }

            // Close all pipe fds in the parent
            for (int i = 0; i < 2 * (num_commands - 1); i++) {
                close(pipe_fds[i]);
            }

            // Wait for all child processes
            for (int i = 0; i < num_commands; i++) {
                wait(NULL);
            }
        } else {
            printf("This example focuses on piping. Please enter a command with '|'.\n");
        }

        for (int k = 0; k < num_commands; k++) free(commands[k]);
        printf("Enter a command with pipes (or 'exit'):\n");
    }

    return 0;
}
