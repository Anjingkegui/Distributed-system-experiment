#ifndef MQUEUE_H
#define MQUEUE_H

#include <sys/ipc.h>

enum msg_type {

    MSG_ANY,

    MSG_SERVER_NAME,
    MSG_FREE_NAME,

    MSG_REGISTER,  /* foer --> master */
    MSG_REGISTER_ACK,
    MSG_UNREGISTER,  /* foer --> master */
    MSG_LOCK_SYNC,     /* master -->  foer */
    MSG_LOCK_NEW,
    MSG_LOCK_DELETE,

    MSG_LOCK_REQUEST,  /* client --> server */
    MSG_LOCK_RELEASE,
    MSG_LOCK_INRUIRY,

    MSG_LOCK_GRANT,   /* server -->  client */
    MSG_LOCK_BUSY,
    MSG_LOCK_IDLE,
    MSG_LOCK_OWN


};

#define MASTER_NODE "999999999"
#define DNS_NODE "999999998"

int mq_open(char *mq_name, int mqflg);
int mq_close(int mq_id);
int mq_read(int mq_id, int msg_type, char *msg_buff, int msg_size);
int mq_write(int mq_id, int msg_type, char *msg_buff, int msg_size);

void publish_name(int mq);
void publish_name(int mq, char *name);
void publish_server(int mq, char *server);
void lookup_name(int mq, char *name, int len);
void lookup_server(int mq, char *server, int len);

#endif 

