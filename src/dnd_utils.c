#include "common.h"
#include "dnd.h"
#include "sessions.h"
#include "dnd_utils.h"

#define MAX_IP_LENGTH 64           // Maximum Length for IP address string
#define MAX_ERRMSG_LEN 256         // Maximum length for error messages
#define MAX_RESPONSE_LEN 300       // Maximum length for response buffer

// Validates whether a given number is a valid 10-digit number
int validate_number(const char *num) {
    if (!num) {
        return 0;
    }

    int len = strlen(num);
    if (len != 10) {
        return 0;
    }

    for (int i = 0; i < len; i++) {
        if (num[i] < '0' || num[i] > '9') {
            return 0;
        }
    }

    return 1;
}

// Handles communication with the connected clients
static void handle_client(int client_fd) {
    char buf[MAX_LINE]; // Buffer array for incoming data
    ssize_t n;

    // Read data from client until connection is closed
    while ((n = recv(client_fd, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        buf[strcspn(buf, "\r\n")] = '\0';

        if (strlen(buf) == 0) {
            continue; // Ignore empty lines
        }

        char *saveptr;
        // Tokenizer to extract command
        char *cmd = strtok_r(buf, " ", &saveptr);
        if (!cmd) {
            continue;
        }

        if (strcasecmp(cmd, "HELLO") == 0) {
            char *num = strtok_r(NULL, " ", &saveptr);
            if (num && *num) {
                (void)sessions_set_subscriber(client_fd, num); // Register the subscriber
            }
            send(client_fd, "OK HELLO\n", 9, 0);
            continue;
        }

        if (strcasecmp(cmd, "ACTIVATE") == 0) {
            char *num = strtok_r(NULL, " ", &saveptr);
            char *type = strtok_r(NULL, " ", &saveptr);
            char *blocked = strtok_r(NULL, " ", &saveptr);

            if (!num || !type) {
                send(client_fd, "ERR Missing params\n", 19, 0);
                continue;
            }

            if (!validate_number(num)) {
                send(client_fd, "ERR Invalid subscriber number\n", 30, 0);
                continue;
            }

            // Parse DND type
            dnd_type_t t = parse_type(type);
            char blocked_arr[MAX_BLOCKED][MAX_NUM_LEN]; // List of blocked numbers
            int count = 0;

            if (blocked && t == DND_SELECTIVE) {
                char *q = blocked, *b;
                while ((b = strsep(&q, ",|")) && count < MAX_BLOCKED) {
                    if (strlen(b) == 0) {
                        continue;
                    }
                    strncpy(blocked_arr[count++], b, MAX_NUM_LEN);
                }
            }

            char errmsg[MAX_ERRMSG_LEN];
            // Try to activate DND
            if (activate_dnd(num, t, blocked_arr, count, errmsg, sizeof(errmsg)) == 0) {
                (void)sessions_set_subscriber(client_fd, num);
                send(client_fd, "OK Activated\n", 13, 0);
            } else {
                char out[MAX_RESPONSE_LEN];
                snprintf(out, sizeof(out), "ERR %s\n", errmsg);
                send(client_fd, out, strlen(out), 0);
            }

        } else if (strcasecmp(cmd, "DEACTIVATE") == 0) {
            char *num = strtok_r(NULL, " ", &saveptr);
            if (!num) {
                send(client_fd, "ERR Missing number\n", 20, 0);
                continue;
            }

            if (!validate_number(num)) {
                send(client_fd, "ERR Invalid subscriber number\n", 30, 0);
                continue;
            }

            char errmsg[MAX_ERRMSG_LEN];
            // Try to deactivate DND
            if (deactivate_dnd(num, errmsg, sizeof(errmsg)) == 0) {
                (void)sessions_set_subscriber(client_fd, num);
                send(client_fd, "OK Deactivated\n", 15, 0);
            } else {
                char out[MAX_ERRMSG_LEN];
                snprintf(out, sizeof(out), "ERR %s\n", errmsg);
                send(client_fd, out, strlen(out), 0);
            }

        } else if (strcasecmp(cmd, "MAKECALL") == 0) {
            char *from = strtok_r(NULL, " ", &saveptr);
            char *to = strtok_r(NULL, " ", &saveptr);

            if (strcmp(from, to) == 0) {
                send(client_fd, "ERR Cannot call yourself\n", 25, 0);
                continue;
            }

            if (!from || !to) {
                send(client_fd, "ERR Missing params\n", 19, 0);
                continue;
            }

            if (!validate_number(from) || !validate_number(to)) {
                send(client_fd, "ERR Invalid number\n", 20, 0);
                continue;
            }

            char res[MAX_ERRMSG_LEN];
            (void)make_call(from, to, res, sizeof(res)); // Simulate call
            strcat(res, "\n");
            send(client_fd, res, strlen(res), 0);  // Track caller

            (void)sessions_set_subscriber(client_fd, from);

        } else if (strcasecmp(cmd, "EXIT") == 0) { // Handle Exit command
            send(client_fd, "OK Bye\n", 7, 0);
            break;

        } else {
            send(client_fd, "ERR Unknown command\n", 21, 0);
        }
    }

    (void)sessions_remove(client_fd);  // Clean up session after work
    close(client_fd);                  // Close the socket connection
}

// Entry Point for each client thread
void *client_thread(void *arg) {
    int fd = *(int *)arg;   // Socket file descriptor
    free(arg);              // DMA
    handle_client(fd);
    return NULL;
}

// This function is designed to be passed to pthread_create() as entry for new thread.
