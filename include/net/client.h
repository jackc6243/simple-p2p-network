#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

// Function to create a connection to the server
int create_connection(char* ip, int port);

#endif