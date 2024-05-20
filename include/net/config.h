#ifndef CONFIG_FILE
#define CONFIG_FILE

#define MAX_STR_LEN 1024

struct config {
    char* directory;
    int max_peers;
    uint16_t port;
    int status;
};

struct config* parse_config(char* file_path);

#endif