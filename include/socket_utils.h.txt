#ifndef SOCKET_UTILS_H
#define SOCKET_UTILS_H

// Create a client socket and connect to the given IP and port
int create_and_connect_socket(const char *ip, int port);

// Create a server socket that listens on the given port
int create_listening_socket(int port);

#endif // SOCKET_UTILS_H
