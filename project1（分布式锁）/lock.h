#ifndef LOCK_H
#define LOCK_H
#include "message.h"


class Dlock {
    public:
        Dlock();
        ~Dlock();

        bool TryLock(std::string lk);
        bool TryUnlock(std::string lk);
        bool isOwnLock(std::string lk);


    private:
        void lk_sendrcv(int type, std::string data);

        int ns_id, m_id, id;
        int uuid;
        struct msg_t msg;
        struct client_t *pclient;
};

#endif
