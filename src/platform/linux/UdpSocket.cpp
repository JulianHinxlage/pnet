//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#include "socket/UdpSocket.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <poll.h>

namespace sock {

    class UdpSocket::Impl{
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

        Error create(){
            if(fd != -1){
                close();
            }

            fd = ::socket(AF_INET6, SOCK_DGRAM, 0);
            if(fd == -1){
                return Error(ErrorCode::ERROR, strerror(errno));
            }

            return Error();
        }

    };

    UdpSocket::UdpSocket() {
        impl = std::make_shared<Impl>();
    }

    Error UdpSocket::listen(uint16_t port) {
        Error error = impl->create();
        if(error){
            return error;
        }

        struct sockaddr_in6 addr;
        addr.sin6_family = AF_INET6;
        addr.sin6_addr = IN6ADDR_ANY_INIT;
        addr.sin6_port = htons(port);

        if(::bind(impl->fd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
            impl->close();
            return Error(ErrorCode::BIND_FAIL, strerror(errno));
        }

        return Error();
    }

    Error UdpSocket::write(const void *ptr, int bytes, const Endpoint &destination) {
        if(impl->fd == -1){
            Error error = impl->create();
            if(error){
                return error;
            }
        }

        if(::sendto(impl->fd, ptr, bytes, 0, (struct sockaddr*)destination.getHandle(), sizeof(sockaddr_in6)) == -1){
            impl->close();
            if(errno == ECONNRESET){
                return Error(ErrorCode::DISCONNECT, "disconnect");
            }
            return Error(ErrorCode::ERROR, strerror(errno));

        }
        return Error();
    }

    Error UdpSocket::read(void *ptr, int &bytes, Endpoint &source, int millisTimeout, bool peek) {
        if(impl->fd == -1){
            Error error = impl->create();
            if(error){
                return error;
            }
        }

        struct pollfd poll;
        poll.fd = impl->fd;
        poll.events = POLLIN;
        int code = ::poll(&poll, 1, millisTimeout);
        if(code == -1){
            bytes = 0;
            impl->close();
            return Error(ErrorCode::ERROR, strerror(errno));
        }else if(code == 0){
            bytes = 0;
            return Error(ErrorCode::TIMEOUT, "timeout");
        }else{
            socklen_t size = sizeof(sockaddr_in6);
            bytes = ::recvfrom(impl->fd, ptr, bytes, peek ? MSG_PEEK : 0, (struct sockaddr*)source.getHandle(), &size);
            if(bytes == -1){
                impl->close();
                bytes = 0;
                return Error(ErrorCode::ERROR, strerror(errno));
            }else if(bytes == 0){
                impl->close();
                return Error(ErrorCode::DISCONNECT, "disconnect");
            }
            return Error();
        }
    }

    Error UdpSocket::readAll(std::vector<char> &buffer, int &bytes, Endpoint &source, int millisTimeout) {
        if(buffer.size() < 8){
            buffer.resize(8);
        }

        bytes = buffer.size();
        Error error = read(buffer.data(), bytes, source, millisTimeout);
        if(error){
            return error;
        }

        while(bytes == buffer.size()){
            buffer.resize(buffer.size() * 1.5);
            int readBytes = buffer.size() - bytes;
            Endpoint source2;
            error = read(&buffer[bytes], readBytes, source2, 0, true);

            if(error){
                if(error == TIMEOUT){
                    break;
                }else{
                    return error;
                }
            }

            if(source != source2){
                break;
            }else{
                error = read(&buffer[bytes], readBytes, source2, 0);
            }

            bytes += readBytes;
            if(error){
                if(error == TIMEOUT){
                    break;
                }else{
                    return error;
                }
            }
        }
        return Error();
    }

    void UdpSocket::shutdown() {
        impl->close();
    }

    int &UdpSocket::getHandle() {
        return impl->fd;
    }

}
