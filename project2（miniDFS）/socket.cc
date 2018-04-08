#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <boost/asio.hpp>

#include "socket.h"
//#define TEST_SOCKET 1

boost::asio::io_service Socket::io_service;


#ifdef TEST_SOCKET

void client(Socket sk)
{
    
    for (;;) {
        char data[16] = "hello";

        sk.write(data, strlen(data));

        sk.read(data, 3);
        data[3] = 0;
        std::cout << "client: " << data << std::endl;
    }

}

void server()
{
    Accepter ac(2333);

    Socket sk = ac.accept();
    while (1) {

        static int i = 0;
        char data[16] = {0};
        sk.read(data, 5);
        std::cout << "server: " << data << i ++ << std::endl;

        sk.write((char *)"bye", 3);
    }
}

int main()
{
    std::thread t2(server);
    sleep(2);
    Socket sk;
    sk.connect("127.0.0.1", 2333);
    std::thread t1(client, std::move(sk));
    t1.join();
    t2.join();

    return 0;
}



#endif
