#include "sessions.h"
#include "common.h"

#define ISO8601_TIME_LEN 32  // Buffer size for ISO 8601 formatted timestamps

// Global array to store session entries
static session_entry_t g_sessions[MAX_SESSIONS];

// Mutex to protect access to the session array in multithreaded environment
static pthread_mutex_t g_sess_mx = PTHREAD_MUTEX_INITIALIZER;

// Convert a time_t value to ISO 8601 format string (2025-19-13T01:31:00Z)
static void iso8601(time_t t, char *out, size_t n) {
    struct tm tm;
    gmtime_r(&t, &tm);  // Convert time to UTC
    strftime(out, n, "%Y-%m-%dT%H:%M:%SZ", &tm); // Format time as ISO 8601
}

// Save all active sessions to a CSV file (sessions.csv); path is provided
// Must be called within mutex locked
static int save_sessions_locked(void) {
    FILE *f = fopen(SESSIONS_PATH, "w");
    if (!f) {
        return -1;
    }

    fprintf(f, "fd,ip,port,subscriber,connected_at,updated_at\n");

    // Write each active session
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!g_sessions[i].in_use) {
            continue;
        }

        char cts[ISO8601_TIME_LEN], uts[ISO8601_TIME_LEN];  // Connection time, last updated time
        iso8601(g_sessions[i].connected_at, cts, sizeof cts);
        iso8601(g_sessions[i].updated_at, uts, sizeof uts);

        // Write data to CSV
        fprintf(f, "%d,%s,%d,%s,%s,%s\n",
                g_sessions[i].fd,
                g_sessions[i].ip,
                g_sessions[i].port,
                g_sessions[i].subscriber,
                cts, uts);
    }

    fclose(f);
    return 0;
}

// Initialize the session system
int sessions_init(void) {
    pthread_mutex_lock(&g_sess_mx); // Lock mutex to protect array
    memset(g_sessions, 0, sizeof g_sessions);  // Clear all session entries

    /* Ensure directory exists from your setup; file is created on first add */
    FILE *f = fopen(SESSIONS_PATH, "w");
    if (f) {
        fprintf(f, "fd,ip,port,subscriber,connected_at,updated_at\n");
        fclose(f);
    }

    pthread_mutex_unlock(&g_sess_mx);  // Unlock mutex
    return 0;
}

// Adds a new session entry for connected client
int sessions_add(int fd, const char *ip, int port, const char *subscriber) {
    pthread_mutex_lock(&g_sess_mx);  // Mutex lock

    int idx = -1;
    // Find first available slot
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (!g_sessions[i].in_use) {
            idx = i;
            break;
        }
    }

    // No available slot
    if (idx < 0) {
        pthread_mutex_unlock(&g_sess_mx);
        return -1;
    }

    // Fill session entries
    g_sessions[idx].in_use = 1;
    g_sessions[idx].fd = fd;
    snprintf(g_sessions[idx].ip, sizeof g_sessions[idx].ip, "%s", ip ? ip : "0.0.0.0");
    g_sessions[idx].port = port;
    snprintf(g_sessions[idx].subscriber, sizeof g_sessions[idx].subscriber, "%s",
             subscriber ? subscriber : "UNKNOWN");
    time(&g_sessions[idx].connected_at);  // Connection time
    g_sessions[idx].updated_at = g_sessions[idx].connected_at;

    int rc = save_sessions_locked(); // Save to file
    pthread_mutex_unlock(&g_sess_mx); // Unlock mutex
    return rc;
}

// Find the index of a session by file descriptor
// Does not lock mutex; must be called inside locked context
static int find_idx_by_fd_nolock(int fd) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (g_sessions[i].in_use && g_sessions[i].fd == fd) { // Found
            return i;
        }
    }

    return -1;
}

// Update the subscriber name for given session
int sessions_set_subscriber(int fd, const char *subscriber) {
    pthread_mutex_lock(&g_sess_mx);  // Lock mutex

    int i = find_idx_by_fd_nolock(fd);  // Find session index
    if (i < 0) {
        pthread_mutex_unlock(&g_sess_mx);
        return -1;
    }

    if (subscriber && *subscriber) {
        snprintf(g_sessions[i].subscriber, sizeof g_sessions[i].subscriber, "%s", subscriber);
    }

    time(&g_sessions[i].updated_at);  // Update time
    int rc = save_sessions_locked();  // Save to file

    pthread_mutex_unlock(&g_sess_mx);  // Unlock mutex
    return rc;
}

// Updates the "updated_at" timestamp for a session
int sessions_touch(int fd) {
    pthread_mutex_lock(&g_sess_mx);  // Lock

    int i = find_idx_by_fd_nolock(fd);  // Find session index
    if (i < 0) {
        pthread_mutex_unlock(&g_sess_mx);
        return -1;
    }

    time(&g_sessions[i].updated_at); // Update timestamp
    int rc = save_sessions_locked();  // Save to file

    pthread_mutex_unlock(&g_sess_mx);  // Unlock
    return rc;
}

// Remove session entry after client disconnected
int sessions_remove(int fd) {
    pthread_mutex_lock(&g_sess_mx);  // Lock

    int i = find_idx_by_fd_nolock(fd);  // Find session index
    if (i >= 0) {
        memset(&g_sessions[i], 0, sizeof g_sessions[i]);  // Clear session entry
    }

    int rc = save_sessions_locked();
    pthread_mutex_unlock(&g_sess_mx);  // Unlock
    return rc;
}

// Clear all session entries and save an empty session list
int sessions_shutdown(void) {
    pthread_mutex_lock(&g_sess_mx);
    memset(g_sessions, 0, sizeof g_sessions);  // Clear all sessions
    int rc = save_sessions_locked(); // Save to file
    pthread_mutex_unlock(&g_sess_mx);
    return rc;
}
