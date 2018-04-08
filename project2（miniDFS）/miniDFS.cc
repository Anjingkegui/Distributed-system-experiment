#include <iostream>
#include <sys/stat.h>
#include <sys/mman.h>

#include "miniDFS.h"
#include "msgdelivery.h"
#include "segment.h"

int minidfs::Connect(std::string ip, int port)
{
    _sock.connect(ip, port);
    return 0;
}
void minidfs::Disconnect()
{
    _sock.close();
}

msgcontainer minidfs::RequireLocation(std::string file, int segno, int fd) 
{

    filelocation floc;
    floc.set_file(file);
    floc.set_segno(segno);
    floc.set_fd(fd); 

    msgcontainer msg;
    msg.set_msgtype(msgcontainer::FILELOCATION);
    msg.set_allocated_floc(&floc);

    writemsg(_sock, msg);
    msg.release_floc();

    readmsg(_sock, msg);

    return msg;
}

void minidfs::notify_filerange(Socket &sk, std::string file, int fileID, int segno, int offset, int length, msgcontainer_type msgtype) 
{
        filerange frange;
        frange.set_file(file);
		frange.set_fileid(fileID);
        frange.set_segno(segno);
        frange.set_offset(offset);
        frange.set_length(length);
		frange.set_ismsgfromcl(true);

        std::cout << "cl: locate seg#" << segno << std::endl;

        msgcontainer msg;
        msg.set_msgtype(msgtype);
        msg.set_allocated_frange(&frange);

        writemsg(sk, msg);
        msg.release_frange();
}

int minidfs::UploadSegment(std::string file, int segno, int segsize, char *segdata)
{
    int fd = -1;

    auto msg = RequireLocation(file, segno);
    if (file == "")
        file = msg.floc().file();

    if (msg.msgtype() == msgcontainer::FILELOCATION) {
        
        fd = msg.floc().fd();
		// now it known upload segment to which dataserver
        Socket sk;
        sk.connect(msg.floc().ip(), msg.floc().port()); 

		// upload segment to dataserver
        notify_filerange(sk, file, fd, segno, 0, segsize, msgcontainer::FILEWRITE);

        sk.write(segdata, segsize);
        sk.close();
    }

    return fd;
}

int minidfs::UploadFile(std::string file)
{
    int length; 
    int fd = -1;
    int id = -1;

    //get length of file
    struct stat st;
    stat(file.data(), &st);
    length = st.st_size;

    
    //map file into memory
    fd = open(file.data(), O_RDONLY);
    if (fd == -1)
        perror("fail to open file");

    char *fdata = (char *)mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
    if (fdata == MAP_FAILED) {
        perror("mmap failed");
        return 0;
    }

    for (int i = 0; i <= (length >> SEG_SHIFT); ++ i) {

        int segsize = std::min(SEG_SIZE, length - i * SEG_SIZE);
        char* segdata = fdata + i * SEG_SIZE;

        id = UploadSegment(file, i, segsize, segdata);
    }

    munmap(fdata, length);
    close(fd);
    return id;
}


void minidfs::ReadSegment(int fd, int segno, int offset, int length, char *buff)
{
    auto msg = RequireLocation("", segno, fd);
    if (msg.msgtype() == msgcontainer::FILELOCATION) {
        fd = msg.floc().fd();

        Socket sk;
        sk.connect(msg.floc().ip(), msg.floc().port()); 

        notify_filerange(sk, msg.floc().file(), fd,  segno, offset, length, msgcontainer::FILEREAD);

        sk.read(buff, length);
        sk.close();
    }
}

int minidfs::Read(int fd, int offset, int length, char *buff)
{
   char *p = buff; 

   while (length > 0) {
       int off_in_seg = offset & (~ SEG_MASK);
       int segno = offset >> SEG_SHIFT;
       int len_in_seg = std::min(length - off_in_seg, (int)SEG_SIZE - off_in_seg);

       ReadSegment(fd, segno, off_in_seg, len_in_seg, p);

       offset += len_in_seg;
       length -= len_in_seg;
       p += len_in_seg;
   }
   return 0;
}
