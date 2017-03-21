#ifndef _NAMESERVER_H
#define _NAMESERVER_H


#include <vector>
#include <map>
#include <mutex>

#include "message.pb.h"
#include "socket.h"

using namespace std;

struct svraddr {
	int id;
    std::string ip; 
    int port;
	Socket *avrSocket;
    svraddr():avrSocket((Socket *)0){}
};

class nameserver {
    public:
        nameserver(int port): _ac(port), current_svr(0), fileidcounter(0) {}
        void running();

    struct fileless {
        bool operator() (const filelocation &lhs, const filelocation &rhs)
        {
            return (lhs.fd() < rhs.fd()) ||
                    (lhs.fd() == rhs.fd() && lhs.segno() < rhs.segno());
        }
    };

    private:
        void message_process(Socket sk);

        void FindSegment(Socket &sk, msgcontainer &msg);

		void DSRegister(Socket &sk, msgcontainer &msg);

        Accepter _ac;
        std::vector<struct svraddr> srvs;
        int current_svr;
        std::map<filelocation, char, nameserver::fileless> files;
        std::mutex floc_lock;
		std::map<string, int> idmap;
		int fileidcounter;
};

#endif
