#ifndef SESSIONS_H
#define SESSIONS_H

#include "common.h"

#define SESSIONS_PATH "data/sessions.csv"
#define MAX_SESSIONS 1024
#define IP_STR_LEN 64
#define SUBSCR_LEN 64

typedef struct{
        int in_use;
        int fd;
        char ip[IP_STR_LEN];
        int port;
        char subscriber[SUBSCR_LEN];
        time_t connected_at;
        time_t updated_at;
}  session_entry_t;

int sessions_init(void);
int sessions_add(int fd,const char *ip,int port,const char *subscriber);
int sessions_set_subscriber(int fd,const char *subscriber);
int sessions_touch(int fd);
int sessions_remove(int fd);
int sessions_shutdown(void);
#endif
