#include <iostream>
#include <algorithm>
#include <thread>

#include "nameserver.h"
#include "msgdelivery.h"


void nameserver::FindFile(Socket &sk, msgcontainer &msg)
{

    floc_lock.lock();

    string file = msg.floc().file();
    auto floc = msg.floc();

    // map file to id
    if (file != "") {
        auto idget = idmap.find(file); 

        if (idget == idmap.end()) { // get a new file id
            idget = idmap.insert(idmap.begin(), std::pair<string, int>(floc.file(), fileidcounter ++));
        }

		floc.set_fd(idget->second);
    }

    // check if file location exsits  key<id, location>
    auto it = files.find(floc.fd());
	
    // new location & keep it in our dictionary
    if (it == files.end()) {
        while (srvs.empty()) {
            sleep(1);
        }

        floc.set_ip(srvs[current_svr].ip);
        floc.set_port(srvs[current_svr].port);
		// round robin
		current_svr = (current_svr + 1) % srvs.size();
        
        it = files.insert(files.begin(), std::pair<int, filelocation>(floc.fd(), floc));
    }
    // now, we must have a location in our dictionary
    floc = it->second;

    floc_lock.unlock();
 
    msg.set_allocated_floc(&floc);
    writemsg(sk, msg);
    msg.release_floc();
}


void nameserver::DeleteFile(Socket &sk, msgcontainer &msg)
{
    floc_lock.lock();

    int thisID = -1;

    string file = msg.filedel().file();

    // map file to id
    if (file != "") {
        auto idget = idmap.find(file); 

        if (idget == idmap.end()) { 
            printf("the file does not exsit.\n" );
            return;
        }
        else{   // delete
            thisID = idget->second;
            idmap.erase(idget);
        }
    }

    auto it = files.find(thisID);
    if (it == files.end()) { 
            printf("the file id does not exsit.\n" );
            return;
        }
        else{   // delete
            files.erase(it);
        }
    
    floc_lock.unlock();
    msg.release_floc();
}


void nameserver::SendAllInfo(Socket &sk, msgcontainer &msg)
{
    int itemNum = files.size();

    auto acluster = msg.acluster();
    acluster.set_clusternum (itemNum);
    msg.set_allocated_acluster(&acluster);
    writemsg(sk, msg);
    msg.release_acluster();

    auto it = files.begin();
    for(; it!=files.end(); ++it)
    {
        msgcontainer tmpMsg;
        filelocation tmpFloc;
        tmpFloc = it->second;

        tmpMsg.set_msgtype(msgcontainer::FILELOCATION);
        tmpMsg.set_allocated_floc(&tmpFloc);
        writemsg(sk, tmpMsg);
        tmpMsg.release_floc();
    }
}


void nameserver::DSRegister(Socket &sk, msgcontainer &msg)
{
	dataserverregister dregister = msg.dregister();
	
	// srvs is a vector keep all the data server s id, ip, port, and socket
	svraddr newSvr, fSvr, eSvr;

	newSvr.id = srvs.size();
	newSvr.ip = dregister.ip();
	newSvr.port = dregister.port();
	newSvr.avrSocket = &sk;
	srvs.push_back(newSvr);

	msgcontainer replayMsg;
	replayMsg.set_msgtype(msgcontainer::DATASERVERINFO);

    if (srvs.size() < 2)  return ;

    dataserverinfo dinfo;

    dinfo.set_fip(srvs[srvs.size() - 2].ip);
    dinfo.set_fport(srvs[srvs.size() - 2].port);
    dinfo.set_eip(srvs[0].ip);
    dinfo.set_eport(srvs[0].port);

    replayMsg.set_allocated_dinfo(&dinfo);
    writemsg(sk, replayMsg);
    replayMsg.release_dinfo();

    dinfo.clear_fip();
    dinfo.clear_fport();
    dinfo.set_eip(newSvr.ip);
    dinfo.set_eport(newSvr.port);

    replayMsg.set_allocated_dinfo(&dinfo);
    writemsg(*(srvs[srvs.size() - 2].avrSocket), replayMsg);
    replayMsg.release_dinfo();

    dinfo.set_fip(newSvr.ip);
    dinfo.set_fport(newSvr.port);
    dinfo.clear_eip();
    dinfo.clear_eport();

    replayMsg.set_allocated_dinfo(&dinfo);
    writemsg(*(srvs[0].avrSocket), replayMsg);
    replayMsg.release_dinfo();

}


void nameserver::message_process(Socket sk)
{
        msgcontainer msg;
        while (readmsg(sk, msg) != -1) {

            switch (msg.msgtype()) {
				// client ask for FILELOCATION
                case msgcontainer::FILELOCATION:
                    FindFile(sk, msg);
                    break;

                // client delete file
                case msgcontainer::DELETEFILE:
                    DeleteFile(sk, msg);
                    break;

                // client aquire all information
                case msgcontainer::ALLINFORMATION:
                    SendAllInfo(sk, msg);
                    break;

				// data server registers
				case msgcontainer::DATASERVERREGISTER:
					DSRegister(sk, msg);
					break;

                default:
                    std::cout << "ns: msg unknown" << std::endl;
                    break;
            }
        }
        sk.close();
}

void nameserver::running()
{
    for(;;) {
        Socket sk = _ac.accept();

        std::thread t(&nameserver::message_process, this, move(sk));

        t.detach();
    }
}
