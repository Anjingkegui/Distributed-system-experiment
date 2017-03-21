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

    private:
        void message_process(Socket sk);

        void FindFile(Socket &sk, msgcontainer &msg);

        void DeleteFile(Socket &sk, msgcontainer &msg);

        void SendAllInfo(Socket &sk, msgcontainer &msg);

		void DSRegister(Socket &sk, msgcontainer &msg);

        Accepter _ac;
        std::vector<struct svraddr> srvs;
        int current_svr;
        std::map<int, filelocation> files;
        std::mutex floc_lock;
		std::map<string, int> idmap;
		int fileidcounter;
};

#endif
