#include <iostream>
#include <algorithm>
#include <thread>

#include "nameserver.h"
#include "msgdelivery.h"


void nameserver::FindSegment(Socket &sk, msgcontainer &msg)
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

    // check if file location exsits  key<id, segno>
    auto it = files.find(floc);

    // new location & keep it in our dictionary
    if (it == files.end()) {
        while (srvs.empty()) {
            sleep(1);
        }

        floc.set_ip(srvs[current_svr].ip);
        floc.set_port(srvs[current_svr].port);
        // round robin
        current_svr = (current_svr + 1) % srvs.size();

        it = files.insert(files.begin(), std::pair<filelocation, char>(floc, '0'));
    }
    // now, we must have a location in our dictionary
    floc = it->first;

    floc_lock.unlock();

    msg.set_allocated_floc(&floc);
    writemsg(sk, msg);
    msg.release_floc();
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
                FindSegment(sk, msg);
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
