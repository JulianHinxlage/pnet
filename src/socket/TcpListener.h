//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#ifndef SOCKET_TCPLISTENER_H
#define SOCKET_TCPLISTENER_H

#include "TcpSocket.h"
#include <memory>

namespace sock {

    class TcpListener {
    public:
        TcpListener();
        Error listen(uint16_t port);
        Error accept(TcpSocket &socket);
        void shutdown();
        int &getHandle();
        bool isListening();
    private:
        class Impl;
        std::shared_ptr<Impl> impl;
    };

}

#endif //SOCKET_TCPLISTENER_H
