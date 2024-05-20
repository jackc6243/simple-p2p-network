#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Function to create a listener socket
int create_listener(int port);

#endif // SERVER_H