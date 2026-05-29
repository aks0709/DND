#include "common.h"
#include "dnd.h"
#include "sessions.h"
#include "dnd_utils.h"
#include "socket_utils.h"  // User-defined library to create and manage sockets
#define MAX_IP_LENGTH 64

int main(int argc, char **argv) {
    int port = LISTEN_PORT;  // LISTEN_PORT is a macro defined in dnd.h

    if (argc > 1) {
        port = atoi(argv[1]); // Override port if provided in command line
    }

    (void)load_db();         // Function call to load database
    (void)sessions_init();   // Initialize session tracking

    int sockfd = create_listening_socket(port); // Create TCP listening socket
    if (sockfd < 0) {
        fprintf(stderr, "Failed to create listening socket\n");
        return 1;
    }

    printf("DND Server listening on port %d...\n", port);

    while (true) {
        struct sockaddr_in cli;
        socklen_t len = sizeof(cli);

        int *client_fd = malloc(sizeof(int));  // Allocate memory for client socket descriptor
        if (client_fd == NULL) {
            perror("malloc failed");
            exit(EXIT_FAILURE);
        }

        *client_fd = accept(sockfd, (struct sockaddr *)&cli, &len); // Accept incoming connection

        char ip[MAX_IP_LENGTH] = {0}; // Buffer array to store client IP
        (void)inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof ip); // Binary to string conversion
        int cport = (int)ntohs(cli.sin_port); // Convert client port from network to host byte order

        (void)sessions_add(*client_fd, ip, cport, "UNKNOWN"); // Add client to session tracker with default name

        pthread_t tid;
        memset(&tid, 0, sizeof(tid)); // Zero-initialize thread identifier
        pthread_create(&tid, NULL, client_thread, client_fd); // Spawn a new thread to handle the client
        pthread_detach(tid);  // Detach thread so it cleans up automatically
    }

    close(sockfd);
    return 0;
}
