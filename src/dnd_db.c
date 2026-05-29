#define _GNU_SOURCE
#include "common.h"
#include "dnd.h"

static client_entry_t clients[MAX_CLIENTS];   /* Array to store client entries */
static int client_count = 0;                  /* Tracks clients currently loaded in memory */

/* This function parses a single line of text from a database file */
static void parse_line(const char *line, client_entry_t *e) {
    char *tmp = strdup(line);         /* Duplicate of the input line to safely modify it */
    char *p = tmp;
    char *num = strsep(&p, ",");
    char *active = strsep(&p, ",");
    char *type = strsep(&p, ",");
    char *blocked = strsep(&p, ",");

    memset(e, 0, sizeof(*e));   /* Initialize the client entry to 0 */

    if (num) {
        strncpy(e->number, num, MAX_NUM_LEN - 1);
        e->number[MAX_NUM_LEN - 1] = '\0';  /* Ensure null termination */
    }

    e->active = active ? atoi(active) : 0;

    if (type) {
        if (strcasecmp(type, "GLOBAL") == 0) {
            e->type = DND_GLOBAL;
        } else if (strcasecmp(type, "SELECTIVE") == 0) {
            e->type = DND_SELECTIVE;
        } else {
            e->type = DND_NONE;
        }
    }

    e->blocked_count = 0;
    if (blocked && strlen(blocked) > 0) {
        char *q = blocked;
        char *b;
        while ((b = strsep(&q, "|")) && e->blocked_count < MAX_BLOCKED) {
            if (strlen(b) == 0) {
                continue;
            }
            strncpy(e->blocked[e->blocked_count], b, MAX_NUM_LEN - 1);
            e->blocked[e->blocked_count][MAX_NUM_LEN - 1] = '\0';
            e->blocked_count++;
        }
    }

    free(tmp); /* Free duplicated line memory */
}

/* Loads client data from the database into memory */
int load_db() {
    pthread_mutex_lock(&db_mutex);

    FILE *f = fopen(DB_PATH, "r");
    if (!f) {
        printf("[WARNING] Could not open DB file: %s\n", DB_PATH);
        fflush(stdout);
        pthread_mutex_unlock(&db_mutex);
        return 0;
    }

    printf("[INFO] Loading database from %s\n", DB_PATH);
    fflush(stdout);

    client_count = 0;
    char line[MAX_LINE];

    while (fgets(line, sizeof(line), f)) {
        if (client_count >= MAX_CLIENTS) {
            break;
        }
        if (line[0] == '#' || strlen(line) < 3) {
            continue;
        }

        line[strcspn(line, "\r\n")] = 0; /* Remove newline */
        parse_line(line, &clients[client_count++]);   /* Parse and store */
    }

    fclose(f);
    printf("[INFO] Loaded %d client entries\n", client_count);
    fflush(stdout);
    pthread_mutex_unlock(&db_mutex);
    return 0;
}

/* Saves the current memory data back to the database */
int save_db() {
    printf("[DEBUG] Entered save_db()\n");
    fflush(stdout);

    FILE *f = fopen(DB_PATH, "w"); /* Open database file for writing */
    if (!f) {
        printf("[FATAL] Failed to open DB file for writing: %s\n", DB_PATH);
        fflush(stdout);
        perror("fopen");
        return -1;
    }

    printf("[DEBUG] Writing database to %s\n", DB_PATH);
    fflush(stdout);

    fprintf(f, "number,active,type,blocked_list\n");

    for (int i = 0; i < client_count; i++) {
        fprintf(f, "%s,%d,%s,", clients[i].number, clients[i].active,
                type_to_str(clients[i].type)); /* Write client information */

        for (int j = 0; j < clients[i].blocked_count; j++) {
            fprintf(f, "%s", clients[i].blocked[j]);
            if (j < clients[i].blocked_count - 1) {
                fprintf(f, "|"); /* Separate blocked numbers */
            }
        }

        fprintf(f, "\n");
    }

    fclose(f);
    printf("[DEBUG] Database successfully saved to %s\n", DB_PATH);
    fflush(stdout);
    return 0;
}

/* Searches for the client entry by the number */
client_entry_t* find_entry(const char *number) {
    for (int i = 0; i < client_count; i++) {
        if (strcmp(clients[i].number, number) == 0) {
            return &clients[i];
        }
    }

    return NULL;
}

/* Finds an existing client or creates a new one if not found */
client_entry_t* find_or_create(const char *number) {
    client_entry_t *e = find_entry(number);
    if (e) {
        return e;
    }

    if (client_count >= MAX_CLIENTS) {
        return NULL;
    }

    client_entry_t *n = &clients[client_count++];
    memset(n, 0, sizeof(*n));
    strncpy(n->number, number, MAX_NUM_LEN - 1);
    n->number[MAX_NUM_LEN - 1] = '\0';
    n->type = DND_NONE;
    n->active = 0;

    printf("[INFO] Created new client entry for %s\n", number);
    fflush(stdout);
    return n;
}

/* Updates the existing client entry and saves to the database */
int update_entry(const client_entry_t *entry) {
    pthread_mutex_lock(&db_mutex);

    client_entry_t *e = find_entry(entry->number);
    if (!e) {
        printf("[WARNING] update_entry: entry not found for %s\n", entry->number);
        fflush(stdout);
        pthread_mutex_unlock(&db_mutex);
        return -1;
    }

    *e = *entry;
    int rc = save_db();

    pthread_mutex_unlock(&db_mutex);
    return rc;
}
