#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include "mqueue.h"
#include "message.h"


/*
 * @name    should be a number, i.e. "1245" 
 * @mqflg   set IPC_CREAT | IPC_EXCL to get
 *          a system unique key
 */

int mq_open(char *name, int mqflg) 
{
    key_t key = (key_t)atoi(name);
    int fd = 0;
    fd = msgget(key, 0666 | mqflg | IPC_CREAT);//| IPC_EXCL);
    if (fd == -1)
        perror("mq_open");

    //printf("open %s fd %d\n", name, fd);
    return fd;
}

int mq_close(int fd)
{
    int e = msgctl(fd, IPC_RMID, NULL);
    //if (e == -1)
     //   perror("mq_close");
    return e;
}

int  mq_read(int fd, int msgtype, char *msgdata, int len)
{
     int e = msgrcv(fd, msgdata, len, msgtype, 0);
     //if (e == -1) 
      //   perror("mq_read");
     return e;

}

int  mq_write(int fd, int msgtype, char *msgdata, int len)
{
    int e ;

    /*hope this helpful*/
    while ((e= msgsnd(fd, msgdata, len, IPC_NOWAIT)) == -1)
        usleep(10);
     //if (e == -1) {
      //   perror("mq_read");
     return e;
}


void publish_name(int mq)
{
    static int name = atoi(DNS_NODE) - 1;
    struct msg_t msg;
    msg.type = MSG_FREE_NAME;
    sprintf(msg.data, "%d", name);
    mq_write(mq, msg.type, (char *)&msg, strlen(msg.data));
    -- name;
}

void publish_name(int mq, char *name)
{
    struct msg_t msg;
    msg.type = MSG_FREE_NAME;
    strcpy(msg.data, name);
    mq_write(mq, msg.type, (char *)&msg, strlen(msg.data));
}

void publish_server(int mq, char *server)
{
    struct msg_t msg;
    msg.type = MSG_SERVER_NAME;
    strcpy(msg.data, server);
    mq_write(mq, msg.type, (char *)&msg, strlen(msg.data));
}

void lookup_server(int mq, char *server, int len)
{
    struct msg_t msg;
    msg.type = MSG_SERVER_NAME;
    mq_read(mq, msg.type, (char *)&msg, MAX_MSG_LEN);
    strcpy(server, msg.data);
}

void lookup_name(int mq, char *name, int len)
{
    struct msg_t msg;
    msg.type = MSG_FREE_NAME;
    mq_read(mq, msg.type, (char *)&msg, MAX_MSG_LEN);
    strcpy(name, msg.data);
}
