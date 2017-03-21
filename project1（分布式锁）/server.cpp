#include <stdio.h>
#include <string>
#include <map>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/wait.h>

#include "mqueue.h"
#include "message.h"

using namespace std;


map<string, int> lock_dict; //<lock, uuid>
map<int, int> mq_dict;   //<uuid, mq_id>
vector<int> mq_foers;    // mq_id

int ns_id;

int sig_mqueue;

void sigkill(int sig)
{
    mq_close(sig_mqueue);
    exit(0);
}

void inquire_lock_record(struct msg_t *pmsg)
{

    struct client_t *p = (struct client_t *)pmsg->data;

    auto it = lock_dict.find(p->lock);
    if (it != lock_dict.end() && it->second == p->uuid) {
        pmsg->type = MSG_LOCK_OWN;
    } else {
        pmsg->type = MSG_LOCK_BUSY;
    }

    int client_mq = mq_dict.find(p->uuid)->second;
    mq_write(client_mq, pmsg->type, (char *)pmsg, sizeof(client_t));
}

void release_lock(struct msg_t *pmsg)
{
    struct client_t *p = (struct client_t *)pmsg->data;
    int client_mq = mq_dict.find(p->uuid)->second;
    map<string, int>::iterator it;

    it = lock_dict.find(p->lock);
    if (it != lock_dict.end() && it->second == p->uuid) {
        //printf("release lock(%s) from client(%d)\n", p->lock, p->uuid);
        pmsg->type = MSG_LOCK_IDLE;
        mq_write(client_mq, pmsg->type, (char *)pmsg, sizeof(client_t));
        lock_dict.erase(it);
        /* sync to foers */
        pmsg->type = MSG_LOCK_DELETE;
        for (auto it = mq_foers.begin(); it != mq_foers.end(); ++ it) {
            mq_write(*it, pmsg->type, (char *)pmsg, sizeof(client_t));
        }

    } else {
        //printf("cannot release lock(%s) from client(%d)\n", p->lock, p->uuid);
        pmsg->type = MSG_LOCK_BUSY;
        mq_write(client_mq, pmsg->type, (char *)pmsg, sizeof(client_t));
    }
}

void request_lock(struct msg_t *pmsg)
{
    struct client_t *p = (struct client_t *)pmsg->data;
    map<string, int>::iterator it;

    it = lock_dict.find(p->lock);
    if (it == lock_dict.end()) {
        /* grant this lock */
        printf("grant lcok(%s) to %d\n", p->lock, p->uuid);
        lock_dict.insert(pair<string, int>(p->lock, p->uuid));

        pmsg->type = MSG_LOCK_GRANT;
        int client_mq = mq_dict.find(p->uuid)->second;
        mq_write(client_mq, MSG_LOCK_GRANT, (char *)pmsg, sizeof(client_t));

        /* sync lock_dict */
        pmsg->type = MSG_LOCK_NEW;
        for (auto it = mq_foers.begin(); it != mq_foers.end(); ++ it) {
            mq_write(*it, pmsg->type, (char *)pmsg, sizeof(client_t));
        }

    } else {
        /* refuse */
        printf("refuse to grant lcok(%s) to %d\n", p->lock, p->uuid);
        pmsg->type = MSG_LOCK_BUSY;
        int client_mq = mq_dict.find(p->uuid)->second;
        mq_write(client_mq, MSG_LOCK_BUSY, (char *)pmsg, sizeof(client_t));
    }

}

void client_register(struct msg_t *pmsg, int &uuid)
{
    struct client_t *p = (struct client_t *)pmsg->data;
    int fd = mq_open(p->name, 0);
    publish_name(ns_id);
    
    if (p->type == FOER) { 
        mq_foers.push_back(fd);
        publish_server(ns_id, p->name);
        /* sync all lock records in lock_dict */
        for (auto &elem : lock_dict) {

            struct msg_t lock_msg = *pmsg;
            struct client_t *lk = (struct client_t *)lock_msg.data;
            lk->type = MSG_LOCK_SYNC;
            strcpy(lk->lock, elem.first.c_str());
            lk->uuid = elem.second;

            for (auto it = mq_foers.begin(); it != mq_foers.end(); ++ it) {
                mq_write(*it, lk->type, (char *)&lock_msg, sizeof(client_t));
            }
        }

    } else { 
        p->uuid = uuid ++;
        pmsg->type = MSG_REGISTER_ACK;

        mq_dict.insert(pair<int, int>(p->uuid, fd));
        mq_write(fd, pmsg->type, (char *)pmsg, sizeof(client_t));
        publish_server(ns_id, p->server);
    }
}

void client_unregister(struct msg_t *pmsg)
{
    struct client_t *p = (struct client_t *)pmsg->data;

    /* release locks own by the client
     * FIXME:is it ok to erase iterator of a map
     */
    for (auto it = lock_dict.begin(); it != lock_dict.end(); ++ it)
        if (it->second == p->uuid) {

            struct msg_t rm_msg = *pmsg;
            struct client_t *rm = (struct client_t *)rm_msg.data;
            rm->type = MSG_LOCK_DELETE;
            strcpy(rm->lock, it->first.c_str());

            for (auto it = mq_foers.begin(); it != mq_foers.end(); ++ it) {
                mq_write(*it, rm->type, (char *)&rm_msg, sizeof(client_t));
            }

            lock_dict.erase(it);
        }

    mq_dict.erase(p->uuid);
    publish_name(ns_id, p->name);
}


int master_run()
{
    int mg_id = mq_open((char *)MASTER_NODE,IPC_CREAT); 
    ns_id = mq_open((char *)DNS_NODE,IPC_CREAT); 
    sig_mqueue = ns_id;
    
    struct msg_t msg;
    static int uuid = 1;

    for (int i = 0; i < 9; ++ i) 
        publish_name(ns_id);
    publish_server(ns_id, (char *)MASTER_NODE);

    while(mq_read(mg_id, MSG_ANY, (char *)&msg, MAX_MSG_LEN) != -1) {

        switch (msg.type) {

            case MSG_REGISTER: 
                client_register(&msg, uuid);
                break;

            case MSG_UNREGISTER: 
                client_unregister(&msg);
                break;

            case MSG_LOCK_REQUEST: 
               request_lock(&msg); 
               break;

            case MSG_LOCK_RELEASE: 
                release_lock(&msg);
                break;

            case MSG_LOCK_INRUIRY: 
                inquire_lock_record(&msg);
                break;

            default:
                printf("shouldn't get here %d\n", msg.type);
                break;
        }
    }

    return 0;
}

/* transfer clients' message to master node */
int message2master(int master_id, msg_t *pmsg, int len)
{
    return mq_write(master_id, pmsg->type, (char *)pmsg, len);
}

void add_lock_record(struct msg_t *pmsg)
{
    struct client_t *p= (struct client_t*)pmsg->data;
    lock_dict.insert(pair<string,int>(p->lock,p->uuid));

}

void delete_lock_record(struct msg_t *pmsg)
{
    struct client_t *p= (struct client_t*)pmsg->data;
    lock_dict.erase(p->lock);
}

void record_client(struct msg_t *pmsg)
{
    struct client_t *p = (struct client_t *)pmsg->data;
    if (p->uuid == -1) return ;

    auto it = mq_dict.find(p->uuid);
    if (it == mq_dict.end()) {
        int fd = mq_open(p->name, 0);
        mq_dict.insert(pair<int, int>(p->uuid, fd));
    }
        
}
void remove_client_in_record(struct msg_t *pmsg)
{
    struct client_t *p = (struct client_t *)pmsg->data;
    if (p->uuid == -1) return ;

    mq_dict.erase(p->uuid);
}

void sync_lock_record(struct msg_t *pmsg)
{
    /*now one message has just one record*/
    add_lock_record(pmsg);
}

int follower_run()
{
    ns_id = mq_open((char *)DNS_NODE,IPC_CREAT); 
    struct msg_t msg;
    
    
    struct client_t *pclient = (struct client_t *)msg.data;

    //lookup_server(ns_id, pclient->server,  sizeof(pclient->server));
    lookup_name(ns_id, pclient->name, sizeof(pclient->name));

    int master_id = mq_open((char *)MASTER_NODE, IPC_CREAT);
    int foer_id = mq_open(pclient->name, IPC_CREAT | IPC_EXCL);
    sig_mqueue = foer_id;

     pclient->type = 1;
     msg.type = MSG_REGISTER;
     mq_write(master_id, MSG_REGISTER, (char *)&msg, sizeof(client_t));

    while(mq_read(foer_id, MSG_ANY, (char *)&msg, MAX_MSG_LEN) != -1) {
        switch (msg.type) {
            case MSG_UNREGISTER:
                remove_client_in_record(&msg);
                /* go down */

            case MSG_REGISTER:
                message2master(master_id, &msg, sizeof(struct client_t));
                break;

            case MSG_LOCK_REQUEST:
            case MSG_LOCK_RELEASE:
                message2master(master_id, &msg, sizeof(struct client_t));
                break;

            case MSG_LOCK_INRUIRY: 
                record_client(&msg);
                inquire_lock_record(&msg);
                break;

            case MSG_LOCK_SYNC: /* sync */
                sync_lock_record(&msg);
                break;
            case MSG_LOCK_NEW:
                add_lock_record(&msg);
                break;
            case MSG_LOCK_DELETE:
                delete_lock_record(&msg);
                break;

            default:
                break;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 1) 
        printf("need a parameter\n");

    signal(SIGINT, sigkill);

    int isMaster = !atoi(argv[1]);
    if (isMaster)
        master_run();
    else
        follower_run();

    return 0;

}


