//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#include "socket/Endpoint.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>

namespace sock {

    class Endpoint::Impl{
    public:
        struct sockaddr_in6 addr;
    };

    Endpoint::Endpoint() {
        impl = std::make_shared<Impl>();
    }

    Endpoint::Endpoint(const char *address, uint16_t port, bool resolve)
            : Endpoint() {
        set(address, port, resolve);
    }

    Endpoint::Endpoint(const Endpoint &ep)
            : Endpoint() {
        impl->addr = ep.impl->addr;
    }

    Endpoint &Endpoint::operator=(const Endpoint &ep) {
        impl->addr = ep.impl->addr;
        return *this;
    }

    bool Endpoint::operator==(const Endpoint &ep) const{
        if(impl->addr.sin6_family != ep.impl->addr.sin6_family){
            return false;
        }

        if(impl->addr.sin6_family == AF_INET){
            struct sockaddr_in *addr = (struct sockaddr_in*)&impl->addr;
            struct sockaddr_in *addr2 = (struct sockaddr_in*)&ep.impl->addr;
            return addr->sin_port == addr2->sin_port && addr->sin_addr.s_addr == addr2->sin_addr.s_addr;
        }


        if(impl->addr.sin6_family == AF_INET6){
            if(impl->addr.sin6_port != ep.impl->addr.sin6_port){
                return false;
            }
            return memcmp(&impl->addr.sin6_addr, &ep.impl->addr.sin6_addr, sizeof(impl->addr.sin6_addr)) == 0;
        }

        return false;
    }

    bool Endpoint::operator!=(const Endpoint &ep) const{
        return !operator==(ep);
    }

    uint16_t Endpoint::getPort() const{
        struct sockaddr_in *addr = (struct sockaddr_in*)&impl->addr;
        return addr->sin_port;
    }

    void Endpoint::setPort(uint16_t port) {
        struct sockaddr_in *addr = (struct sockaddr_in*)&impl->addr;
        addr->sin_port = port;
    }

    const char *Endpoint::getAddress() const{
        static char buf[INET6_ADDRSTRLEN];
        if(isv4()){
            return inet_ntop(impl->addr.sin6_family, &((sockaddr_in*)&impl->addr)->sin_addr, buf, sizeof(buf));
        }else{
            return inet_ntop(impl->addr.sin6_family, &impl->addr.sin6_addr, buf, sizeof(buf));
        }
    }

    void Endpoint::setAddress(const char *address, bool resolve) {
        impl->addr.sin6_family = 0;

        if(inet_pton(AF_INET, address, &((sockaddr_in*)&impl->addr)->sin_addr) == 1){
            impl->addr.sin6_family = AF_INET;
            return;
        }

        if(inet_pton(AF_INET6, address, &impl->addr.sin6_addr) == 1){
            impl->addr.sin6_family = AF_INET6;
            return;
        }

        if(resolve){
            struct addrinfo *info;
            if(getaddrinfo(address, nullptr, nullptr, &info) == 0){
                for(addrinfo *i = info; i != nullptr; i = i->ai_next){
                    if(i->ai_family == AF_INET){
                        *(sockaddr*)&impl->addr = *i->ai_addr;
                    }else if (i->ai_family == AF_INET6){
                        impl->addr = *(sockaddr_in6*)i->ai_addr;
                    }
                    break;
                }
                freeaddrinfo(info);
            }
        }
    }

    void Endpoint::set(const char *address, unsigned short port, bool resolve) {
        setAddress(address, resolve);
        setPort(port);
    }

    bool Endpoint::valid() const{
        return impl->addr.sin6_family == AF_INET || impl->addr.sin6_family == AF_INET6;
    }

    bool Endpoint::isv4() const{
        return impl->addr.sin6_family == AF_INET;
    }

    bool Endpoint::isv6() const{
        return impl->addr.sin6_family == AF_INET6;
    }

    void *Endpoint::getHandle() const{
        return &impl->addr;
    }

}
