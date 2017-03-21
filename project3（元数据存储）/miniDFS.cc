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
int minidfs::Disconnect()
{
    _sock.close();
    return 0;
}
msgcontainer minidfs::RequireLocation(std::string file, int length ) 
{

    filelocation floc;
    floc.set_file(file);
    floc.set_filelen(length);

    msgcontainer msg;
    msg.set_msgtype(msgcontainer::FILELOCATION);
    msg.set_allocated_floc(&floc);

    socket_lock.lock();
    writemsg(_sock, msg);
    msg.release_floc();

    readmsg(_sock, msg);
    socket_lock.unlock();

    return msg;
}
int minidfs::notify_filerange(Socket &sk, std::string file, int fileID, int length, msgcontainer_type msgtype) 
{
        filerange frange;
        frange.set_file(file);
		frange.set_fileid(fileID);
        frange.set_length(length);
		frange.set_ismsgfromcl(true);

        msgcontainer msg;
        msg.set_msgtype(msgtype);
        msg.set_allocated_frange(&frange);

        writemsg(sk, msg);
        msg.release_frange();

        return 0;
}


//----------------------------------------------------------------------------
// upload file
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
        perror(("fail to open file " + file).data());


    char *fdata = (char *)mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, 0);
    if (fdata == MAP_FAILED) {
        perror("mmap failed");
        return 0;
    }

    // get arrangement
    // the reply will be msgcontainer::FILELOCATION too
    auto msg = RequireLocation(file, length);
    if (file == "")
        file = msg.floc().file();

    if (msg.msgtype() == msgcontainer::FILELOCATION) {
        
        id = msg.floc().fd();
        // now it known upload file to which dataserver
        Socket sk;
        sk.connect(msg.floc().ip(), msg.floc().port()); 

        // upload file to dataserver
        notify_filerange(sk, file, id, length, msgcontainer::FILEWRITE);

        sk.write(fdata, length);
        sk.close();
    }

    munmap(fdata, length);
    close(fd);
    return id;
}


//----------------------------------------------------------------------------
// read file
// buff get space in the func, but should be delete outside
int minidfs::ReadFile(std::string file, char **buff)
{
    auto msg = RequireLocation(file);

    if (msg.msgtype() == msgcontainer::FILELOCATION) {
        // now it known upload file to which dataserver
        Socket sk;
        sk.connect(msg.floc().ip(), msg.floc().port()); 

        // upload file to dataserver
        notify_filerange(sk, file, msg.floc().fd(), msg.floc().filelen(), msgcontainer::FILEREAD);

        *buff = new char[msg.floc().filelen()];
        sk.read(*buff, msg.floc().filelen());
        sk.close();
    }

    return 0;
}


//----------------------------------------------------------------------------
// delete file
int minidfs::DeleteFile(std::string file)
{
    filedelete filedel;
    filedel.set_file(file);

    msgcontainer msg;
    msg.set_msgtype(msgcontainer::DELETEFILE);
    msg.set_allocated_filedel(&filedel);

    socket_lock.lock();
    writemsg(_sock, msg);
    socket_lock.unlock();

    msg.release_filedel();

    return 0;
}


//----------------------------------------------------------------------------
// get all info
int minidfs::GetClusterInfo()
{
    clusterdata acluster;

    msgcontainer msg;
    msg.set_msgtype(msgcontainer::ALLINFORMATION);
    msg.set_allocated_acluster(&acluster);

    socket_lock.lock();

    // inform the nameserver to get all info data
    writemsg(_sock, msg);
    msg.release_acluster();
    readmsg(_sock, msg);

    int itemNum = msg.acluster().clusternum();

    FILE *fp; 
    fp =  fopen("clusterData.txt", "w");

    for(int i=0; i<itemNum; i++)
    {
        msgcontainer tmpMsg;
        readmsg(_sock, tmpMsg);
        // tmpMsg is type of filelocation

        filelocation tmpFloc = tmpMsg.floc();
        // translate the msg into our form

        fputs(tmpFloc.file().c_str(), fp);
        fputs(tmpFloc.ip().c_str(), fp);

        /*
        char tmpPort[10];
        itoa(tmpFloc.port(),tmpPort,10);
        fputs(tmpPort, fp);
        */
        fprintf(fp, "%d", tmpFloc.port());


        fputc('\n',fp); 
    }
    fclose(fp);
    socket_lock.unlock();
    return 1;
}


