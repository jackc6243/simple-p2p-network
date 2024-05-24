#include <stdio.h>

int main(int argc, char** argv) {
    char buffer[1024];

    printf("argv[1]: %s\n", argv[1]);

    while (fgets(buffer, 1024, stdin)) {
        printf("echo: %s\n", buffer);
    }

    return 0;
}