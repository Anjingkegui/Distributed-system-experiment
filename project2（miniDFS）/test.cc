#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include "message.pb.h"

using namespace std;
using boost::asio::ip::tcp;

boost::asio::io_service io_service;

void client(msgcontainer msg)
{
    tcp::socket sock(io_service);

    string data;
    msg.SerializeToString(&data);

    sock.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 2333));
    //sock.write_some(boost::asio::buffer(data));
    cout << "client write bytes #" << boost::asio::write(sock, boost::asio::buffer(data)) << endl;
    sock.close();
}

void server()
{
    tcp::acceptor a(io_service, tcp::endpoint(tcp::v4(), 2333));
    {
        tcp::socket sock(io_service);
        a.accept(sock);

        char data[1024];
        boost::system::error_code error;
        size_t length = sock.read_some(boost::asio::buffer(data), error);
        cout << "server read bytes #" << length << endl;
        
        msgcontainer msg;
        msg.ParseFromArray(data, length);
        msg.release_fsize();
        sock.close();
    }
}

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    msgcontainer msg;
    filesize fsize;

    
    fsize.set_name("test.cc");
    fsize.set_size(10);

    msg.set_msgtype(msgcontainer::FILESIZE);
    msg.set_allocated_fsize(&fsize);

    cout << msg.fsize().name() << " " << msg.fsize().size() << endl;
    cout << "main " << msg.SerializeAsString().length() << endl;

    thread t1(client, std::ref(msg));
    thread t2(server);

    msg.release_fsize();
    t1.join();
    t2.join();


    return 0;
}


