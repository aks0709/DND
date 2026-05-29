#include "common.h"
#include "socket_utils.h"

#define BACKLOG 10  // Max number of pending connections

// Creates TCP socket and connects to server using given port and IP
// Returns socket file descriptor
int create_and_connect_socket(const char *ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in serv; // Structure to hold server address
    serv.sin_family = AF_INET; // Use IPv4
    serv.sin_port = htons(port); // Port to network byte order

    if (inet_pton(AF_INET, ip, &serv.sin_addr) != 1) { // IP: string to binary
        fprintf(stderr, "Invalid IP\n");
        close(sock);
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv, sizeof(serv)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }

    return sock;
}

// Creates a TCP listening socket for specific port
// Returns listening socket descriptor
int create_listening_socket(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in serv = {0};  // Initialize with zero
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;  // Accept connection on any local IP
    serv.sin_port = htons(port);

    // Bind socket to given address and port
    if (bind(sockfd, (struct sockaddr *)&serv, sizeof(serv)) < 0) {
        perror("bind");
        close(sockfd);
        return -1;
    }

    // Start listening for connections
    if (listen(sockfd, BACKLOG) < 0) {
        perror("listen");
        close(sockfd);
        return -1;
    }

    return sockfd;
}
