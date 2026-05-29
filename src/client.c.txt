#include "common.h"
#include "config.h"
#include "socket_utils.h"

void print_menu() {
    printf("\n");
    printf(COLOR_CYAN);
    printf("╔════════════════════════════════════╗\n");
    printf("║           📋 DND MENU              ║\n");
    printf("╠════════════════════════════════════╣\n");
    printf("║ " COLOR_GREEN "1. Activate DND" COLOR_CYAN "                    ║\n");
    printf("║ " COLOR_GREEN "2. Deactivate DND" COLOR_CYAN "                  ║\n");
    printf("║ " COLOR_GREEN "3. Make Call" COLOR_CYAN "                       ║\n");
    printf("║ " COLOR_RED   "4. Exit" COLOR_CYAN "                            ║\n");
    printf("╚════════════════════════════════════╝\n");
    printf(COLOR_RESET);
}

// send a commands to the server and prints the response
void send_and_receive(int sock, const char *cmd) {
    char buffer[MAX_RESPONSE_BUFFER];  // buffer to store response
    fflush(stdout);  // flush output buffer to ensure clean prompt

    ssize_t sent = send(sock, cmd, strlen(cmd), 0);  // send command to server
    if (sent < 0) {
        perror("send");
        exit(1);
    }

    ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0); // receive response
    if (n <= 0) {
        printf("Server disconnected.\n");
        exit(0);
    }

    buffer[n] = '\0';
    printf("SERVER: %s", buffer);
}

// entry point of client program, connects to server and interacts via menu driven commands
int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <server-ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *ip = argv[1];  // server ip
    int port = atoi(argv[2]);  // server port number

    int sock = create_and_connect_socket(ip, port);  // function call to connect to server
    if (sock < 0) {
        return EXIT_FAILURE;
    }

    printf("Connected to DND Server at %s:%d\n", ip, port);

    char subscriber[MAX_SUBSCRIBER_LEN];  // buffer for subscriber number
    system("clear");
    printf("Enter your subscriber number: ");
    if (!fgets(subscriber, sizeof(subscriber), stdin)) {
        printf("No input\n");
        close(sock);
        return EXIT_FAILURE;
    }

    subscriber[strcspn(subscriber, "\n")] = '\0';

    {
        char hello[MAX_HELLO_LEN];
        snprintf(hello, sizeof(hello), "HELLO %s\n", subscriber);
        send(sock, hello, strlen(hello), 0);
        char ack[MAX_ACK_LEN];
        recv(sock, ack, sizeof(ack) - 1, 0);
    }

    int choice;
    char mode[MAX_MODE_LEN], blocked[MAX_BLOCKED_LEN], to[MAX_TO_LEN], cmd[MAX_CMD];  // dnd mode, blocked number, receiver number and command buffer

    while (1) {
        print_menu();
        printf("Enter your choice: ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input.\n");
            while (getchar() != '\n') {
                // consume invalid input
            }
            continue;
        }

        while (getchar() != '\n') {
            // consume leftover newline
        }

        switch (choice) {
            case 1: {
                printf("Enter DND mode (" COLOR_GREEN "GLOBAL" COLOR_RESET "/" COLOR_RED "SELECTIVE" COLOR_RESET "): ");
                if (!fgets(mode, sizeof(mode), stdin)) {
                    printf("Input error\n");
                    continue;
                }

                mode[strcspn(mode, "\n")] = '\0';

                if (strcasecmp(mode, "SELECTIVE") == 0) {
                    printf("Enter numbers to block (comma or | separated): ");
                    if (!fgets(blocked, sizeof(blocked), stdin)) {
                        printf("Input error\n");
                        continue;
                    }

                    blocked[strcspn(blocked, "\n")] = '\0';
                    snprintf(cmd, sizeof(cmd), "ACTIVATE %s %s %s\n", subscriber, mode, blocked);
                } else {
                    snprintf(cmd, sizeof(cmd), "ACTIVATE %s %s\n", subscriber, mode);
                }

                send_and_receive(sock, cmd);
                break;
            }

            case 2: {
                snprintf(cmd, sizeof(cmd), "DEACTIVATE %s\n", subscriber);
                send_and_receive(sock, cmd);
                break;
            }

            case 3: {
                printf("Enter receiver number: ");
                if (!fgets(to, sizeof(to), stdin)) {
                    printf("Input error\n");
                    continue;
                }

                to[strcspn(to, "\n")] = '\0';
                snprintf(cmd, sizeof(cmd), "MAKECALL %s %s\n", subscriber, to);
                send_and_receive(sock, cmd);
                break;
            }

            case 4: {
                send(sock, "EXIT\n", 5, 0);
                printf("Disconnected from server.\n");
                close(sock);
                return EXIT_SUCCESS;
            }

            default: {
                printf("Invalid choice.\n");
                break;
            }
        }
    }

    close(sock);
    return EXIT_SUCCESS;
}
