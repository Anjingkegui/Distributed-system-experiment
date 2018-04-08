#ifndef _SOCKET_H
#define _SOCKET_H

#include <boost/asio.hpp>


class Socket 
{

    friend class Accepter;

    public:
        Socket(): _sock(io_service){}

        void connect(std::string ip, int port);
        void close();

        std::size_t read(char *buff, int size);

        std::size_t write(std::string data) {
            return boost::asio::write(_sock, boost::asio::buffer(data));
        }
        std::size_t write(char *data, int length);

    private:
        static boost::asio::io_service io_service;
        boost::asio::ip::tcp::socket _sock;
};

class Accepter {
    public:
        Accepter(int port): _acceptor(Socket::io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)){}

        void accept(Socket &sock);

        Socket accept();

    private:
        boost::asio::ip::tcp::acceptor _acceptor;

};


// implement class Socket
inline void Socket::connect(std::string ip, int port) 
{
    return _sock.connect(boost::asio::ip::tcp::endpoint(
                boost::asio::ip::address::from_string(ip),
                port));
}

inline std::size_t Socket::read(char *buff, int size)
{
    std::size_t sz = 0;
//    return sz = boost::asio::read(_sock, boost::asio::buffer(buff, size));
    boost::system::error_code ec;
    sz = boost::asio::read(_sock, boost::asio::buffer(buff, size), ec);
    if (ec == boost::asio::error::eof) {
        return -1;
    } 
    return sz;

}

inline std::size_t Socket::write(char *data, int length)
{
    std::size_t sz = 0;
    boost::system::error_code ec;
    sz = boost::asio::write(_sock, boost::asio::buffer(data, length), ec);
    if (ec == boost::asio::error::eof) {
        return -1;
    } 
    return sz;
}

inline void Socket::close()
{
    _sock.close();
}

inline void Accepter::accept(Socket &sock)
{
    _acceptor.accept(sock._sock);
}

inline Socket Accepter::accept()
{
    Socket sk;
    _acceptor.accept(sk._sock);
    return sk;
}
#endif


