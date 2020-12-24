//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#ifndef SOCKET_TCPSOCKET_H
#define SOCKET_TCPSOCKET_H

#include "Endpoint.h"
#include "Error.h"
#include <vector>
#include <memory>

namespace pnet {

    class TcpSocket {
    public:
        TcpSocket();
        Error connect(const Endpoint &ep);
        Error connect(const char *address, uint16_t port, bool resolve = false);
        void disconnect();
        Error write(const void *ptr, int bytes);
        Error read(void *ptr, int &bytes, int timeoutMillis = -1);
        Error readAll(std::vector<char> &buffer, int &bytes, int timeoutMillis = -1);
        bool &isConnected() const;
        Endpoint &getEndpoint() const;
        int &getHandle() const;
    private:
        class Impl;
        std::shared_ptr<Impl> impl;
    };

}

#endif //SOCKET_TCPSOCKET_H
