#include <stdio.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include "mqueue.h"
#include "message.h"

using namespace std;

int ns_id;

int sig_mqueue;

void sigkill(int sig)
{
    mq_close(sig_mqueue);
    exit(0);
}

int  app_run(int fd, int master_fd, int uuid)
{
    char cmd[65], lname[65];
    msg_t msg;
    struct client_t *p = (struct client_t *)msg.data;
    p->uuid = uuid;
    
    int pid = fork();
    if (pid == 0) {
        while(mq_read(fd, MSG_ANY, (char *)&msg, MAX_MSG_LEN) != -1) {
            switch (msg.type) {
                case MSG_LOCK_GRANT:
                    printf("client %d: own this lock(%s)\n", uuid, p->lock);
                    break;
                case MSG_LOCK_BUSY:
                    printf("client %d: lock(%s) is busy\n", uuid, p->lock);
                    break;
                case MSG_LOCK_IDLE:
                    printf("client %d: lock(%s) is idle\n", uuid, p->lock);
                    break;
                default:
                    ;
            }
        }
        mq_close(fd);
    } else {
        while (scanf("%s %s", cmd, p->lock) != EOF) {
            if (strcmp(cmd,"req") == 0) {
                msg.type = MSG_LOCK_REQUEST;
                printf("client %d: ready to request lock %s\n", uuid, p->lock);
                mq_write(master_fd, msg.type, (char *)&msg, sizeof(client_t));
            }
            if (strcmp(cmd,"release") == 0) {
                msg.type = MSG_LOCK_RELEASE;
                printf("client %d: ready to release lock %s\n", uuid, p->lock);
                mq_write(master_fd, msg.type, (char *)&msg, sizeof(client_t));
            }
            if (strcmp(cmd,"inquire") == 0) {
                msg.type = MSG_LOCK_INRUIRY;
                printf("client %d: ready to inquire lock %s\n", uuid, p->lock);
                mq_write(master_fd, msg.type, (char *)&msg, sizeof(client_t));
            }
            if (strcmp(cmd, "quit") == 0) {
                msg.type = MSG_UNREGISTER;
                printf("client %d: ready to quit\n", uuid);
                ((struct client_t *)msg.data)->uuid = uuid;
                mq_write(master_fd, msg.type, (char *)&msg, sizeof(client_t));
                break;
            }
        }

        kill(pid, SIGINT);
        wait(NULL);
        mq_close(fd);
    }

    return 0;

}
void print_usages()
{
}

int main(int argc, char *argv[])
{
    int fd, master_fd;
    int uuid;
    struct msg_t msg;
    struct client_t *pclient = (struct client_t *)&msg.data; 
    

    ns_id = mq_open((char *)DNS_NODE,IPC_CREAT); 

    lookup_server(ns_id, pclient->server,  sizeof(pclient->server));
    lookup_name(ns_id, pclient->name, sizeof(pclient->name));

    master_fd = mq_open((char *)pclient->server, IPC_CREAT);
    fd = mq_open(pclient->name, IPC_CREAT | IPC_EXCL);

    sig_mqueue = fd;
    signal(SIGINT, sigkill);
    

     pclient->type = 0;
     msg.type = MSG_REGISTER;

     mq_write(master_fd, MSG_REGISTER, (char *)&msg, sizeof(client_t));
     mq_read(fd, MSG_REGISTER_ACK, (char *)&msg, sizeof(client_t));


     app_run(fd, master_fd, pclient->uuid);

     return 0;
    
}
