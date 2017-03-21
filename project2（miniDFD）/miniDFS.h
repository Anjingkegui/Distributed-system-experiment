#ifndef _MINIDFS_H
#define _MINIDFS_H

#include "socket.h"
#include "message.pb.h"

class minidfs {
    public:
        int Connect(std::string ip, int port);
        int UploadFile(std::string file); 
        int Read(int fd, int offset, int length, char *buff);
        void Disconnect();

    private:

        int UploadSegment(std::string file, int segno, int segsize, char *segdata);
        void ReadSegment(int fd, int segno, int offset, int length, char *buff);
        msgcontainer RequireLocation(std::string file, int segno, int fd = 0);
		void notify_filerange(Socket &sk, std::string  file, int fileID, int segno, int offset, int length, msgcontainer_type msgtype);

		// _sock is used to connect nameserver
    public:
        Socket _sock;
        char _data[512];
        int _size;

};


#endif
