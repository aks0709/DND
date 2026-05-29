#define _GNU_SOURCE
#include "common.h"
#include "dnd.h"

pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER; /* Declares and initializes a mutex to protect shared database */

/* Checks if the caller is in the selective block list */
static int is_blocked_selective(const client_entry_t *e, const char *from) {
    if (!e || !from) {
        return 0;
    }

    for (int i = 0; i < e->blocked_count; i++) {
        if (strcmp(e->blocked[i], from) == 0) {
            return 1;
        }
    }

    return 0;
}

/* Activates DND for a client number */
int activate_dnd(const char *number, dnd_type_t type, char blocked[][MAX_NUM_LEN],
                 int blocked_count, char *errmsg, size_t errlen) {
    if (!number || type == DND_NONE) {
        snprintf(errmsg, errlen, "Invalid parameters");
        return -1;
    }

    printf("[DEBUG] Inside activate_dnd(): number=%s, type=%d\n", number, type);
    fflush(stdout);

    pthread_mutex_lock(&db_mutex);   /* Locks the database for thread-safe access */
    printf("[DEBUG] Mutex locked in activate_dnd()\n");
    fflush(stdout);

    client_entry_t *e = find_or_create(number);
    if (!e) {
        snprintf(errmsg, errlen, "Failed to create/find entry");
        pthread_mutex_unlock(&db_mutex);
        printf("[ERROR] Could not create/find entry for %s\n", number);
        fflush(stdout);
        return -1;
    }

    /* Prevents reactivation of the service without deactivation */
    if (e->active) {
        snprintf(errmsg, errlen, "Already active, deactivate first");
        pthread_mutex_unlock(&db_mutex);
        printf("[WARN] %s already active, skipping\n", number);
        fflush(stdout);
        return -1;
    }

    /* Initial state of client entry before activation */
    e->active = 1;
    e->type = type;
    e->blocked_count = 0;

    printf("[DEBUG] Setting type=%s and blocked_count=%d\n", type_to_str(type), blocked_count);
    fflush(stdout);

    if (type == DND_SELECTIVE) {
        for (int i = 0; i < blocked_count && i < MAX_BLOCKED; i++) {
            strncpy(e->blocked[e->blocked_count], blocked[i], MAX_NUM_LEN - 1);
            e->blocked[e->blocked_count][MAX_NUM_LEN - 1] = '\0';
            printf("[DEBUG] Added blocked number: %s\n", e->blocked[e->blocked_count]);
            e->blocked_count++;
        }
        fflush(stdout);
    }

    /* Saves in the database */
    if (save_db() != 0) {
        snprintf(errmsg, errlen, "Failed to save DB");
        pthread_mutex_unlock(&db_mutex);
        printf("[ERROR] save_db() failed for %s\n", number);
        fflush(stdout);
        return -1;
    }

    /* Unlocks and returns success */
    pthread_mutex_unlock(&db_mutex);
    printf("[DEBUG] activate_dnd() completed successfully for %s\n", number);
    fflush(stdout);
    return 0;
}

/* Deactivates DND for a client entry */
int deactivate_dnd(const char *number, char *errmsg, size_t errlen) {
    if (!number) {
        snprintf(errmsg, errlen, "Invalid number");
        return -1;
    }

    printf("[DEBUG] Inside deactivate_dnd(): number=%s\n", number);
    fflush(stdout);

    pthread_mutex_lock(&db_mutex);
    client_entry_t *e = find_entry(number);

    if (!e) {
        snprintf(errmsg, errlen, "Number not found");
        pthread_mutex_unlock(&db_mutex);
        printf("[WARN] deactivate_dnd(): number not found: %s\n", number);
        fflush(stdout);
        return -1;
    }

    /* Checks if the service is already deactivated */
    if (!e->active) {
        snprintf(errmsg, errlen, "Already deactivated");
        pthread_mutex_unlock(&db_mutex);
        printf("[INFO] deactivate_dnd(): already inactive: %s\n", number);
        fflush(stdout);
        return -1;
    }

    e->active = 0;
    e->type = DND_NONE;
    e->blocked_count = 0;

    if (save_db() != 0) {
        snprintf(errmsg, errlen, "Failed to save DB");
        pthread_mutex_unlock(&db_mutex);
        printf("[ERROR] deactivate_dnd(): save_db failed for %s\n", number);
        fflush(stdout);
        return -1;
    }

    pthread_mutex_unlock(&db_mutex);
    printf("[DEBUG] deactivate_dnd() completed successfully for %s\n", number);
    fflush(stdout);
    return 0;
}

/* Simulates a call from one number to another number */
int make_call(const char *from, const char *to, char *result, size_t rlen) {
    if (!from || !to) {
        snprintf(result, rlen, "ERR Invalid params");
        return -1;
    }

    pthread_mutex_lock(&db_mutex);
    client_entry_t *callee = find_entry(to);

    if (!callee || !callee->active) {
        snprintf(result, rlen, "ESTABLISHED");
        pthread_mutex_unlock(&db_mutex);
        return 0;
    }

    if (callee->type == DND_GLOBAL) {
        snprintf(result, rlen, "BLOCKED_BY_GLOBAL");
    } else if (callee->type == DND_SELECTIVE && is_blocked_selective(callee, from)) {
        snprintf(result, rlen, "BLOCKED_BY_SELECTIVE");
    } else {
        snprintf(result, rlen, "ESTABLISHED");
    }

    pthread_mutex_unlock(&db_mutex);
    return 0;
}

/* Converts a string to an enum */
dnd_type_t parse_type(const char *s) {
    if (!s) {
        return DND_NONE;
    }

    if (strcasecmp(s, "GLOBAL") == 0) {
        return DND_GLOBAL;
    }

    if (strcasecmp(s, "SELECTIVE") == 0) {
        return DND_SELECTIVE;
    }

    if (strcmp(s, "1") == 0) {
        return DND_GLOBAL;
    }

    if (strcmp(s, "2") == 0) {
        return DND_SELECTIVE;
    }

    return DND_NONE;
}

const char* type_to_str(dnd_type_t t) {
    if (t == DND_GLOBAL) {
        return "GLOBAL";
    }

    if (t == DND_SELECTIVE) {
        return "SELECTIVE";
    }

    return "NONE";
}
