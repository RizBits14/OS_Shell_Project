#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_LINE 256
#define MAX_PARTS 128

int main() {
    char line[MAX_LINE];
    char *parts[MAX_PARTS];

    printf("Enter a command with redirection (or 'exit'):\n");
    while (fgets(line, sizeof(line), stdin) != NULL) {
        int len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        if (strcmp(line, "exit") == 0) {
            break;
        }

        int in_fd = -1;
        int out_fd = -1;
        int append = 0;
        int arg_count = 0;
        char *token = strtok(line, " ");

        while (token != NULL && arg_count < MAX_PARTS - 1) {
            if (strcmp(token, "<") == 0) {
                token = strtok(NULL, " ");
                if (token) {
                    in_fd = open(token, O_RDONLY);
                    if (in_fd == -1) {
                        perror("open input file failed");
                        break;
                    }
                } else {
                    fprintf(stderr, "Error: Missing filename after '<'\n");
                    break;
                }
            } else if (strcmp(token, ">") == 0) {
                token = strtok(NULL, " ");
                if (token) {
                    out_fd = open(token, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (out_fd == -1) {
                        perror("open output file failed");
                        break;
                    }
                } else {
                    fprintf(stderr, "Error: Missing filename after '>'\n");
                    break;
                }
            } else if (strcmp(token, ">>") == 0) {
                token = strtok(NULL, " ");
                if (token) {
                    out_fd = open(token, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    if (out_fd == -1) {
                        perror("open append file failed");
                        break;
                    }
                } else {
                    fprintf(stderr, "Error: Missing filename after '>>'\n");
                    break;
                }
            } else {
                parts[arg_count++] = token;
            }
            token = strtok(NULL, " ");
        }
        parts[arg_count] = NULL;

        if (parts[0] != NULL) {
            pid_t pid = fork();

            if (pid == 0) {
                if (in_fd != -1) {
                    dup2(in_fd, STDIN_FILENO);
                    close(in_fd);
                }
                if (out_fd != -1) {
                    dup2(out_fd, STDOUT_FILENO);
                    close(out_fd);
                }
                execvp(parts[0], parts);
                perror("Command not found");
                exit(1);
            } else if (pid > 0) {
                wait(NULL);
                if (in_fd != -1) close(in_fd);
                if (out_fd != -1) close(out_fd);
            } else {
                perror("fork failed");
                if (in_fd != -1) close(in_fd);
                if (out_fd != -1) close(out_fd);
            }
        } else {
            if (in_fd != -1) close(in_fd);
            if (out_fd != -1) close(out_fd);
        }
        printf("Enter a command with redirection (or 'exit'):\n");
    }

    return 0;
}
