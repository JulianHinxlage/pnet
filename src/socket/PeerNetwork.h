//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#ifndef SOCKET_PEERNETWORK_H
#define SOCKET_PEERNETWORK_H

#include "UdpSocket.h"
#include "SocketHandler.h"
#include "Blob.h"
#include <map>

namespace sock {

    typedef Blob<16> PeerId;

    class Peer{
    public:
        PeerId id;
        Endpoint ep;
    };

    class PeerNetwork {
    public:
        enum Opcode{
            NONE,
            PING,
            PONG,
            HANDSHAKE,
            HANDSHAKE_REPLY,
            LOOKUP,
            LOOKUP_REPLY,
            ROUTE,
            BROADCAST,
            DISCONNECT,
        };

        std::vector<Peer> peerCache;
        UdpSocket socket;
        SocketHandler handler;
        uint16_t port;
        std::vector<char> buffer;
        Peer localPeer;
        std::vector<Endpoint> entryNodes;
        std::function<void(PeerId id, const std::string &msg)> msgCallback;
        std::function<void(int level, const std::string &msg)> logCallback;
        std::map<PeerId, std::string> broadcasts;
        bool running;

        PeerNetwork(uint16_t port = 2000);
        Error run();
        void stop();
        void broadcast(const std::string &msg);
        std::string hex(PeerId id, bool shortVersion = false);
    private:
        void onError(Error error);
        void onInfo(const std::string &msg);
        void onDebug(const std::string &msg);

        void addToCache(const Peer &peer);
        const Peer &getNextPeer(PeerId destination, PeerId except = 0);
        bool isInCache(PeerId id);
        void process(int millisTimeout);
        const char *opcodeName(Opcode opcode);
        void disconnect();
    };

}

#endif //SOCKET_PEERNETWORK_H
