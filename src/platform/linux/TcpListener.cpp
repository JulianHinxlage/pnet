//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#include "socket/TcpListener.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

namespace sock {

    class TcpListener::Impl{
    public:
        int fd;

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
        }
    };

    TcpListener::TcpListener() {
        impl = std::make_shared<Impl>();
    }

    Error TcpListener::listen(unsigned short port) {
        if(impl->fd != -1){
            impl->close();
        }

        impl->fd = ::socket(AF_INET6, SOCK_STREAM, 0);
        if(impl->fd == -1){
            return Error(ErrorCode::ERROR, strerror(errno));
        }

        struct sockaddr_in6 addr;
        addr.sin6_family = AF_INET6;
        addr.sin6_addr = IN6ADDR_ANY_INIT;
        addr.sin6_port = htons(port);

        if(::bind(impl->fd, (const sockaddr*)&addr, sizeof(addr)) < 0){
            impl->close();
            return Error(ErrorCode::BIND_FAIL, strerror(errno));
        }

        if(::listen(impl->fd, 10) < 0){
            impl->close();
            return Error(ErrorCode::ERROR, strerror(errno));
        }

        return Error();
    }

    Error TcpListener::accept(TcpSocket &socket) {
        struct sockaddr_in6 addr;
        socklen_t size = sizeof(sockaddr_in6);
        socket.getHandle() = ::accept(impl->fd, (struct sockaddr *)socket.getEndpoint().getHandle(), &size);
        if(socket.getHandle() == -1){
            return Error(ErrorCode::ERROR, strerror(errno));
        }
        socket.isConnected() = true;
        return Error();
    }

    void TcpListener::shutdown() {
        impl->close();
    }

    int &TcpListener::getHandle() {
        return impl->fd;
    }

    bool TcpListener::isListening() {
        return impl->fd != -1;
    }

}
