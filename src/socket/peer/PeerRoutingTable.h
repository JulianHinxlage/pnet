//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#ifndef SOCKET_PEERROUTINGTABLE_H
#define SOCKET_PEERROUTINGTABLE_H

#include "socket/Blob.h"
#include "socket/Endpoint.h"
#include <vector>

namespace sock {

    typedef Blob<16> PeerId;

    class Peer{
    public:
        PeerId id;
        Endpoint ep;
    };

    std::string hex(PeerId id, bool shortVersion = false);

    class PeerRoutingTable{
    public:
        std::vector<Peer> peers;
        Peer defaultPeer;

        PeerRoutingTable();
        Peer &localPeer();
        void add(const Peer &peer);
        void add(const PeerId &id, const Endpoint &ep);
        bool has(const PeerId &id);
        bool has(const Endpoint &ep);
        const Peer &get(const PeerId &id);
        const Peer &get(const Endpoint &ep);
        bool remove(const PeerId &id);
        const Peer &getNext(const PeerId &id, const PeerId &except = 0);
        PeerId lookupTarget(int level);
        int getLevel(PeerId id);
    };

}

#endif //SOCKET_PEERROUTINGTABLE_H
