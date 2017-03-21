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
#include "lock.h"

using namespace std;

void Dlock::lk_sendrcv(int type, string data)
{
    msg.type = type;
    strcpy(pclient->lock, data.c_str());

    mq_write(m_id, msg.type, (char *)&msg, sizeof(client_t));
    mq_read(id, MSG_ANY, (char *)&msg, MAX_MSG_LEN);
}

Dlock::Dlock():pclient((struct client_t *)&msg.data)
{


    ns_id = mq_open((char *)DNS_NODE,IPC_CREAT); 

    lookup_server(ns_id, pclient->server,  sizeof(pclient->server));
    lookup_name(ns_id, pclient->name, sizeof(pclient->name));

    m_id = mq_open((char *)pclient->server, IPC_CREAT);
    id = mq_open(pclient->name, IPC_CREAT | IPC_EXCL);
    
     pclient->type = 0;
     msg.type = MSG_REGISTER;

     mq_write(m_id, MSG_REGISTER, (char *)&msg, sizeof(client_t));
     mq_read(id, MSG_REGISTER_ACK, (char *)&msg, sizeof(client_t));

     uuid = pclient->uuid;
}

bool Dlock::TryLock(string lk)
{
    lk_sendrcv(MSG_LOCK_REQUEST, lk);
    return msg.type == MSG_LOCK_GRANT;
}

bool Dlock::TryUnlock(string lk)
{
    lk_sendrcv(MSG_LOCK_RELEASE, lk);
    return msg.type == MSG_LOCK_IDLE;
}

bool Dlock::isOwnLock(string lk)
{

    lk_sendrcv(MSG_LOCK_INRUIRY, lk);
    return msg.type == MSG_LOCK_OWN;
}

Dlock::~Dlock()
{
    mq_close(id);
}

