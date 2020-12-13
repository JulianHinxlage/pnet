//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#ifndef SOCKET_UDPSOCKET_H
#define SOCKET_UDPSOCKET_H

#include "Endpoint.h"
#include "Error.h"
#include <memory>
#include <vector>

namespace sock {

    class UdpSocket {
    public:
        UdpSocket();
        Error listen(uint16_t port);
        void shutdown();
        Error write(const void *ptr, int bytes, const Endpoint &destination);
        Error read(void *ptr, int &bytes, Endpoint &source, int millisTimeout = -1, bool peek = false);
        Error readAll(std::vector<char> &buffer, int &bytes, Endpoint &source, int millisTimeout = -1);
        int &getHandle();
    private:
        class Impl;
        std::shared_ptr<Impl> impl;
    };

}

#endif //SOCKET_UDPSOCKET_H
