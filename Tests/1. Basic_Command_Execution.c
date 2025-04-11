#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

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

        int j = 0;
        char *token = strtok(line, " ");
        while (token && j < MAX_PARTS - 1) {
            parts[j++] = token;
            token = strtok(NULL, " ");
        }
        parts[j] = NULL;

        if (parts[0] != NULL) {
            pid_t pid = fork();

            if (pid == 0) {
                execvp(parts[0], parts);
                perror("Command not found");
                exit(1);
            } else if (pid > 0) {
                wait(NULL);
            } else {
                perror("fork failed");
            }
        }
    }

    return 0;
}
