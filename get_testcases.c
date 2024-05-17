#include <stdio.h>

int main(int argc, char** argv) {
    FILE* file = fopen(argv[1], "r");
    char buffer[1024] = { 0 };
    while (fgets(buffer, 1024, file)) {
        puts(buffer);
    }
    return 0;
}