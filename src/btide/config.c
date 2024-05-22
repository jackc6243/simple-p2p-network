#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "../../include/net/config.h"
#include "../../include/chk/pkgchk.h"


struct config* parse_config(char* file_path) {
    struct config* args = (struct config*)malloc(sizeof(struct config));
    char buffer[MAX_LINE_LEN];
    FILE* file = fopen(file_path, "r");
    char delim[] = ":\n\r";

    if (file == NULL) {
        return NULL;
    }

    // char* context, tok;
    // fgets(buffer, MAX_LINE_LEN, file);
    // printf("buffer: %s\n");
    // tok = strtok_r(buffer, delim, &context);
    // printf("tok: %s\n");

    if (!parse_info(buffer, file, "directory", delim, (void*)&args->directory, 1, MAX_STR_LEN)) {
        args->status = 3;
    }
    struct stat st = { 0 };
    // Check if the directory exists, if not try creating it
    if (stat(args->directory, &st) == -1 && mkdir(args->directory, 0777) == -1) {
        // Directory does not exist and also failed to create
        args->status = 3;
        fprintf(stderr, "Error creating directory: %s\n", strerror(errno));
        return args;
    }

    int temp = 69;
    int* temp_ptr = &temp;

    if (!parse_info(buffer, file, "max_peers", delim, (void**)&temp_ptr, 0, 0) ||
        (temp < 1 || temp > 2048)) {
        args->status = 4;
        return args;
    }
    args->max_peers = temp;

    if (!parse_info(buffer, file, "port", delim, (void**)&temp_ptr, 0, 0) ||
        (temp <= 1024 || temp > 65535)) {
        args->status = 5;
        return args;
    }
    args->port = temp;
    fclose(file);

    return args;
}