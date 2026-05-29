#ifndef CONFIG_H
#define CONFIG_H

// Buffer sizes
#define MAX_CMD             1024
#define MAX_MODE_LEN        32
#define MAX_BLOCKED_LEN     256
#define MAX_TO_LEN          64
#define MAX_SUBSCRIBER_LEN  64
#define MAX_HELLO_LEN       128
#define MAX_ACK_LEN         128
#define MAX_RESPONSE_BUFFER 1024  // Maximum size for server response buffer

// Exit codes
#define SUCCESS_CODE        EXIT_SUCCESS
#define FAILURE_CODE        EXIT_FAILURE


#endif // CONFIG_H
