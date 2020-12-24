//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#ifndef SOCKET_SOCKETHANDLER_H
#define SOCKET_SOCKETHANDLER_H

#include "TcpSocket.h"
#include "UdpSocket.h"
#include <memory>
#include <functional>

namespace pnet {

    class SocketHandler {
    public:
        SocketHandler();
        void add(int handle, const std::function<void()> &callback);
        void remove(int handle);
        Error run(int timeoutMillis = 100);
        void stop();
    private:
        class Impl;
        std::shared_ptr<Impl> impl;
    };

}

#endif //SOCKET_SOCKETHANDLER_H
