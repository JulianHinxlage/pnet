//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#ifndef SOCKET_PEERNETWORK_H
#define SOCKET_PEERNETWORK_H

#include "PeerRoutingTable.h"
#include "socket/UdpSocket.h"
#include "socket/SocketHandler.h"
#include "socket/Packet.h"
#include <thread>
#include <map>

namespace sock {

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
            MESSAGE,
            DISCONNECT,
        };
        std::function<void(int level, const std::string &msg)> logCallback;
        std::function<void(const PeerId &id, const std::string &msg)> msgCallback;

        PeerNetwork();
        void addEntryNode(const Endpoint &ep);
        Error start(uint16_t port, const char *address = "127.0.0.1");
        void stop();
        Error join();
        void disconnect();
        bool isConnected();
        void broadcast(const std::string &msg);
        void send(const std::string &msg, const PeerId &id);
        PeerId localId();
        void waitForStop();
        const std::vector<Peer> &getPeers();
    private:
        PeerRoutingTable routingTable;
        SocketHandler handler;
        UdpSocket socket;
        std::shared_ptr<std::thread> thread;
        std::vector<char> readBuffer;
        std::vector<Endpoint> entryNodes;
        std::map<Blob<32>, bool> broadcastIds;

        void readPacket(int millisTimeout);
        void processPacket(Packet &packet, const Endpoint &sourceEp);
        void sendPacket(Packet &packet, const PeerId &destination);
        void lookup(const PeerId &target);

        void logError(Error error);
        void log(const std::string &msg, bool debug = false);
    };

}

#endif //SOCKET_PEERNETWORK_H
