#ifndef MESSAGE_H
#define MESSAGE_H

#define MAX_MSG_LEN 256

struct msg_t {
    long type;
    char data[MAX_MSG_LEN];
};

/* TODO: remove this struct 
struct lock_t {
    int client;
    char lock[64];
};
*/
struct ns_t {
    int ns_t;
};

enum client_type {
    CLIENT,
    FOER,
};

struct client_t {
    int type;
    int uuid;
    char name[64];
    char lock[64];
    char server[64];
};



#endif

