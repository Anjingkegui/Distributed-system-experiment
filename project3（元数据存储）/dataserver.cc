#include <algorithm>
#include <thread>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <openssl/md5.h>

#include "dataserver.h"
#include "msgdelivery.h"
#include "segment.h"
#include "message.pb.h"


void dataserver::running()
{

    //just make a new directory
    workingDirectory = "thread" + std::to_string(myPort);
    mkdir(workingDirectory.c_str(), S_IRWXU);

	// connect nameserver
	Socket sk_withNS;
	sk_withNS.connect(nameserverIP, nameserverPort); 
	
	std::thread t1(&dataserver::message_process_nameserver, this, std::move(sk_withNS));
	t1.detach();

	// waiting for connect
	for(;;) {
		Socket sk_withCLorDS = _ac.accept();

		std::thread t(&dataserver::message_process_getdata, this, std::move(sk_withCLorDS));
		t.detach();
	}
}

void dataserver::message_process_nameserver(Socket sk)
{
	// register
	msgcontainer registerMsg;
	registerMsg.set_msgtype(msgcontainer::DATASERVERREGISTER);
	dataserverregister dregister;
	dregister.set_ip(myIP);
	dregister.set_port(myPort);
	registerMsg.set_allocated_dregister(&dregister);
    //test
	if (writemsg(sk, registerMsg) == -1)
        std::cout << "data server: register fail" << std::endl;
	registerMsg.release_dregister();

	// waiting for the massage from nameserver to update neighbors info
	msgcontainer updMsg;
	while(readmsg(sk, updMsg) != -1) {
		updateMyNeighbors(updMsg);
	}
    sk.close();
}

void dataserver::updateMyNeighbors(msgcontainer &updMsg)
{
	dataserverinfo info = updMsg.dinfo();

	if (info.has_fip())
	{
		hasfDataServ = true;
		fDataServer.ip = info.fip();
		fDataServer.port = info.fport();
	}
	if (info.has_eip())
	{
		haseDataServ = true;
		eDataServer.ip = info.eip();
		eDataServer.port = info.eport();
	}
}

void dataserver::message_process_getdata(Socket sk)
{
	// now get the connect
	msgcontainer msg;
	readmsg(sk, msg);
	// message filerange
	// msgtype should be FILEREAD or FILEWRITE
	filerange frange;
	frange = msg.frange();
    
	// frange
	string file = frange.file();
	int fileID = frange.fileid();
	int length = frange.length();

	string segmentFileName = file + to_string(fileID);
    
	char buff[length];// = {0}; // buffer for segement data

	// client wants to read
	if (msg.msgtype() == msgcontainer::FILEREAD) {
		readLocalFile(buff, segmentFileName, length);
		sk.write(buff, length);
	}

	// client or data server wants to write
	else if (msg.msgtype() == msgcontainer::FILEWRITE) {
		// from client
		// get data and store
		// change the flag(isMsgFromCL), pass the data to its neighbors
		if (frange.ismsgfromcl() == true) {

			msgcontainer newmsg;
			filerange newfrange = frange;
			newfrange.set_ismsgfromcl(false);

			newmsg.set_msgtype(msgcontainer::FILEWRITE);
			newmsg.set_allocated_frange(&newfrange);

			sk.read(buff, length);
			saveToLocalFile(buff, segmentFileName, length);
            //cout << "ds: from cl " << segmentFileName << endl;

            if (hasfDataServ) 
			    deliverToNeighbor(fDataServer.ip, fDataServer.port, buff, length, newmsg);
            if (haseDataServ) 
			    deliverToNeighbor(eDataServer.ip, eDataServer.port, buff, length, newmsg);

            newmsg.release_frange();
		} 
		else {
		// from data server
		// just get the data and store
            unsigned char md5[MD5_DIGEST_LENGTH] = {0};

			while(sk.read(buff, length) != -1) {
			    saveToLocalFile(buff, segmentFileName, length);
                MD5((unsigned char*)buff, length, md5);
                sk.write((char *)md5, sizeof(md5));
            }

            //cout << "ds: from ds " << segmentFileName << endl;
		}
	}
    sk.close();
}


void dataserver::readLocalFile(char *buff, string segmentFileName, int length)
{
    string file = workingDirectory + "/" + segmentFileName;
    int fd = open(file.c_str(), O_RDONLY);
    if (fd == -1) {
        perror(("fail to open file " + file + " line:" +  to_string(__LINE__)).data());
        exit(0);
    }
    
    //lseek(fd, offset, SEEK_SET);
    read(fd, buff, length);
    close(fd);
}



void dataserver::deliverToNeighbor(string ip, int port, char *buff, int length, msgcontainer &msg)
{
    unsigned char md5[MD5_DIGEST_LENGTH] = {0};
    unsigned char md5_2[MD5_DIGEST_LENGTH] = {0};
    MD5((unsigned char*)buff, length, md5);

    Socket sk;
    sk.connect(ip, port); 
    writemsg(sk, msg);

    do {
        sk.write(buff, length);
        sk.read((char *)md5_2, sizeof(md5_2));
    } while (memcmp(md5, md5_2, sizeof(md5)) != 0);
    
    sk.close();
}


void dataserver::saveToLocalFile(char *buff, string segmentFileName, int length)
{
    string file = workingDirectory + "/" + segmentFileName;
    int fd = open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror(("fail to open file " + file + " line:" +  to_string(__LINE__)).data());
        exit(0);
    }
    write(fd, buff, length);
    close(fd);
}
