#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

int main(int argc, char** argv) {
    int parent_to_child[2];  // Pipe for stdin redirection
    int child_to_parent[2]; // Pipe for stdout redirection
    pid_t pid;
    char buffer[1024];

    // Create the pipes
    if (pipe2(parent_to_child, O_NONBLOCK) == -1 || pipe2(child_to_parent, O_NONBLOCK) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Fork the process
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Child process
        // Redirect stdin
        dup2(parent_to_child[1], fileno(stdin));
        close(parent_to_child[1]);
        close(parent_to_child[0]);

        // Redirect stdout
        dup2(child_to_parent[0], fileno(stdout));
        close(child_to_parent[0]);
        close(child_to_parent[1]);

        // Execute the target program
        execl("./temp", argv[1], NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else { // Parent process
        // Close unused pipe ends
        close(parent_to_child[0]);
        close(child_to_parent[1]);

        if (argc > 2) {
            // side peer process tests
            FILE* file = fopen("side.in", "r");

            close(child_to_parent[0]);
            sleep(1); // to make sure the main tests are run first
            while (fgets(buffer, 1024, file)) {
                // printf("buffer: %s", buffer);
                write(parent_to_child[1], buffer, strlen(buffer));
            }
            wait(NULL);
            printf("Side peer tests finished");
            fclose(file);
            return 0;
        }

        // main peer process tests
        // Write text from file to the child's stdin
        FILE* file = fopen("main.in", "r");
        while (fgets(buffer, 1024, file)) {
            // printf("buffer: %s", buffer);
            write(parent_to_child[1], buffer, strlen(buffer));
        }

        memset(buffer, '\0', sizeof(buffer)); // reset the buffer

        sleep(2); // wait for btide process to run

        // Read the output from the child's stdout and put it into a file
        int n;
        while ((n = read(child_to_parent[0], buffer, sizeof(buffer) - 1)) > 0) {
            // buffer[n] = '\0'; // Null-terminate the output
            write(fileno(stdout), buffer, n); // Write the output to stdout
        }

        // close resources
        close(child_to_parent[0]);
        close(parent_to_child[1]); // Close the write end to send EOF

        // Wait for the child process to finish
        wait(NULL);
        printf("Main peer tests finished");
        fclose(file);
    }

    return 0;
}