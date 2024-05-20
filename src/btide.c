#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "../include/chk/pkgchk.h"
#include "../include/net/config.h"
#include "../include/net/client.h"
#include "../include/net/server.h"

#define MAX_THREADS 2048
#define MAX_COMMAND 5540

pthread_t thread_ids[MAX_THREADS];


int main(int argc, char** argv) {

    if (argc < 2) {
        puts("Please enter config file");
        return 0;
    }

    // parsing config file
    struct config* config = parse_config(argv[1]);
    if (config == NULL) {
        puts("No such config file");
        return 0;
    }

    if (config->status > 2) {
        exit(config->status);
    }

    char* tok;
    char* context;
    char input[MAX_COMMAND];
    while (1) {
        // Read input from stdin
        if (fgets(input, MAX_COMMAND, stdin) == NULL) {
            continue;
        }
        tok = strtok_r(input, " ", &context);

        // Parsing commands
        if (strcmp(input, "QUIT") == 0) {
            printf("Exiting...\n");
            break;
        } else if (strcmp(input, "CONNECT") == 0) {
            printf("Connecting...\n");
            // Add your connect logic here
        } else if (strcmp(input, "DISCONNECT") == 0) {
            printf("Disconnecting...\n");
            // Add your disconnect logic here
        } else if (strcmp(input, "ADDPACKAGE") == 0) {
            printf("Adding package...\n");
            // Add your add package logic here
        } else if (strcmp(input, "REMPACKAGE") == 0) {
            printf("Removing package...\n");
            // Add your remove package logic here
        } else if (strcmp(input, "PACKAGES") == 0) {
            printf("Listing packages...\n");
            // Add your list packages logic here
        } else if (strcmp(input, "PEERS") == 0) {
            printf("Listing peers...\n");
            // Add your list peers logic here
        } else if (strcmp(input, "FETCH") == 0) {
            printf("Fetching...\n");
            // Add your fetch logic here
        } else {
            printf("Invalid input.\n");
        }

        // Process the input (you can add your code here)
        printf("You entered: %s\n", input);
    }

    free(config->directory);
    free(config);

}
