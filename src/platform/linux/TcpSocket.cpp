//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#include "socket/TcpSocket.h"
#include <sys/socket.h>
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <netinet/in.h>
#include <unistd.h>

namespace sock {

    class TcpSocket::Impl{
    public:
        int fd;
        bool connected;
        Endpoint ep;

        Impl(){
            fd = -1;
        }

        ~Impl(){
            close();
        }

        void close(){
            if(fd != -1){
                ::close(fd);
                fd = -1;
            }
            connected = false;
        }
    };

    TcpSocket::TcpSocket() {
        impl = std::make_shared<Impl>();
    }

    Error TcpSocket::connect(const Endpoint &ep) {
        if(impl->fd != -1){
            impl->close();
        }

        if(!ep.valid()){
            return Error(ErrorCode::ERROR, "invalid Endpoint");
        }

        impl->fd = socket(ep.isv4() ? AF_INET : AF_INET6, SOCK_STREAM, 0);
        if(impl->fd == -1){
            return Error(ErrorCode::ERROR, strerror(errno));
        }

        impl->ep = ep;
        if(::connect(impl->fd, (const sockaddr*)ep.getHandle(), sizeof(sockaddr_in6)) == -1){
            impl->close();
            return Error(ErrorCode::ERROR, strerror(errno));
        }
        impl->connected = true;

        return Error();
    }

    Error TcpSocket::connect(const char *address, unsigned short port, bool resolve) {
        return connect(Endpoint(address, port, resolve));
    }

    void TcpSocket::disconnect() {
        impl->close();
    }

    Error TcpSocket::write(const void *ptr, int bytes) {
        if(::send(impl->fd, ptr, bytes, 0) == -1){
            if(errno == ECONNRESET){
                impl->connected = false;
                return Error(ErrorCode::DISCONNECT, "disconnect");
            }
            return Error(ErrorCode::ERROR, strerror(errno));
        }
        return Error();
    }

    Error TcpSocket::read(void *ptr, int &bytes, int millisTimeout) {
        int code = 1;
        if(millisTimeout != -1){
            struct pollfd poll;
            poll.fd = impl->fd;
            poll.events = POLLIN;
            code = ::poll(&poll, 1, millisTimeout);
        }

        if(code == -1){
            bytes = 0;
            return Error(ErrorCode::ERROR, strerror(errno));
        }else if(code == 0){
            bytes = 0;
            return Error(ErrorCode::TIMEOUT, "timeout");
        }else{
            bytes = ::recv(impl->fd, ptr, bytes, 0);
            if(bytes == -1){
                bytes = 0;
                return Error(ErrorCode::ERROR, strerror(errno));
            }else if(bytes == 0){
                impl->connected = false;
                return Error(ErrorCode::DISCONNECT, "disconnect");
            }
            return Error();
        }
    }

    Error TcpSocket::readAll(std::vector<char> &buffer, int &bytes, int millisTimeout) {
        if(buffer.size() < 8){
            buffer.resize(8);
        }

        bytes = buffer.size();
        Error error = read(buffer.data(), bytes, millisTimeout);
        if(error){
            return error;
        }

        int offset = 0;
        while(bytes == buffer.size() - offset){
            buffer.resize(buffer.size() * 1.5);
            offset += bytes;
            bytes = buffer.size() - offset;
            error = read(&buffer[offset], bytes, 0);
            if(error){
                if(error != TIMEOUT){
                    bytes += offset;
                    return error;
                }
            }
        }
        bytes += offset;
        return Error();
    }

    bool &TcpSocket::isConnected() const{
        return impl->connected;
    }

    Endpoint &TcpSocket::getEndpoint() const{
        return impl->ep;
    }

    int &TcpSocket::getHandle() const {
        return impl->fd;
    }

}
