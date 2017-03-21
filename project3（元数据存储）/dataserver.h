#ifndef _DATASERVER_H
#define _DATASERVER_H


#include <vector>
#include <map>

#include "message.pb.h"
#include "socket.h"

#include "segment.h"

using namespace std;

struct neighborServer {
	std::string ip; 
	int port;
};


// the dataserver
// 1. connect the nameserver to register and keep waiting message from the nameserver 
//    to update the info about its neighbors
// 2. be connected by several clients
//    receive their data and store
//    or
//    receive their request for local data

class dataserver {
public:
	dataserver(string dip, int dport, string nip, int nport): _ac(dport) 
	{
		myIP = dip;
		myPort = dport;
		nameserverIP = nip;
		nameserverPort = nport;
		hasfDataServ = false;
		haseDataServ = false;
	}
	void running();

private:
	void message_process_nameserver(Socket sk);
	void message_process_getdata(Socket sk);

	void updateMyNeighbors(msgcontainer &updMsg);

	void readLocalFile(char *buff, string segmentFileName, int length);
	void deliverToNeighbor(string ip, int port, char *buff, int length, msgcontainer &msg);
	void saveToLocalFile(char *buff, string segmentFileName, int length);

	Accepter _ac;

	string myIP;
	int myPort;
	string nameserverIP;
	int nameserverPort;

	bool hasfDataServ;
	bool haseDataServ;
	neighborServer fDataServer;
	neighborServer eDataServer;

    std::string workingDirectory;
};

#endif
