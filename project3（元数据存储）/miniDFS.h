#ifndef _MINIDFS_H
#define _MINIDFS_H

#include "socket.h"
#include "message.pb.h"

#include <stdio.h>
#include <mutex>
#include <vector>
#include <string>

class minidfs {
    public:
        int Connect(std::string ip, int port);
        
        int UploadFile(std::string file); 
        int ReadFile(std::string file, char **buff);
        int DeleteFile(std::string file);
        int GetClusterInfo();

        int Disconnect();

    private:
        msgcontainer RequireLocation(std::string file, int fd = 0);
		int notify_filerange(Socket &sk, std::string  file, int fileID, int length, msgcontainer_type msgtype);
        std::mutex socket_lock;

		// _sock is used to connect nameserver
    public:
        Socket _sock;
        char _data[512];
        int _size;

};


#endif
