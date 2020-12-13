//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#include "PeerNetwork.h"
#include "Packet.h"
#include <random>

namespace sock {

    PeerNetwork::PeerNetwork(uint16_t port) {
        this->port = port;
        buffer.resize(256);
    }

    Error PeerNetwork::run() {
        running = true;
        Error error = socket.listen(port);
        if (error) {
            return error;
        }

        //random id
        std::srand(std::random_device()());
        for (int i = 0; i < sizeof(localPeer.id); i++) {
            localPeer.id.data[i] = (unsigned char) std::rand();
        }
        localPeer.ep = Endpoint("127.0.0.1", port);

        onInfo("port: " + std::to_string(port));
        onInfo("id: " + hex(localPeer.id));

        //connect to entry node
        for (auto &entry : entryNodes) {
            if (entry != localPeer.ep) {
                Packet response;
                response.add(Opcode::HANDSHAKE);
                response.add(localPeer.id);
                socket.write(response.data(), response.size(), entry);
                process(200);
                if (peerCache.size() > 0) {
                    break;
                }
            }
        }

        handler.add(socket.getHandle(), [&]() {
            process(0);
        });

        if (peerCache.size() > 0) {
            //join lookups
            for (int i = 0; i < sizeof(PeerId) * 8; i++) {
                PeerId dist = (PeerId) 1 << i;
                PeerId target = localPeer.id ^ dist;

                Packet response;
                response.add(Opcode::LOOKUP);
                response.add(localPeer.id);
                response.add(target);
                auto &hop = getNextPeer(target, localPeer.id);
                response.add(hop.id);
                socket.write(response.data(), response.size(), hop.ep);
            }
        } else {
            onDebug("no entry node found");
        }

        if (running) {
            error = handler.run();
        }
        disconnect();
        return error;
    }

    void PeerNetwork::disconnect() {
        //send disconnect message ro every peer in the peer cache
        handler.remove(socket.getHandle());
        Packet packet;
        packet.add(Opcode::DISCONNECT);
        packet.add(localPeer.id);
        for (auto &p : peerCache) {
            socket.write(packet.data(), packet.size(), p.ep);
        }
    }

    void PeerNetwork::process(int millisTimeout) {
        Endpoint hopEp;
        int bytes = 0;
        Error error = socket.readAll(buffer, bytes, hopEp, millisTimeout);
        if (error) {
            if (error != TIMEOUT) {
                onError(error);
            }
            return;
        }

        Packet request(buffer, bytes);

        PeerId hopId = 0;
        for (auto &peer : peerCache) {
            if (peer.ep == hopEp) {
                hopId = peer.id;
                break;
            }
        }

        while (request.size() > 0) {
            int packetStartIndex = request.offset;
            Opcode opcode = request.get<Opcode>();
            PeerId source = request.get<PeerId>();

            onDebug("packet: " + (std::string) opcodeName(opcode) + " " + hex(source) + " [" +
                    std::to_string(hopEp.getPort()) + "]");

            switch (opcode) {
                case PING: {
                    Packet response;
                    response.add(Opcode::PONG);
                    response.add(localPeer.id);
                    socket.write(response.data(), response.size(), hopEp);
                    break;
                }
                case PONG: {
                    break;
                }
                case HANDSHAKE: {
                    addToCache({source, hopEp});
                    Packet response;
                    response.add(Opcode::HANDSHAKE_REPLY);
                    response.add(localPeer.id);
                    socket.write(response.data(), response.size(), hopEp);
                    break;
                }
                case HANDSHAKE_REPLY: {
                    addToCache({source, hopEp});
                    break;
                }
                case LOOKUP: {
                    PeerId target = request.get<PeerId>();
                    PeerId replyHop = request.get<PeerId>();
                    onDebug("destination: " + hex(target));
                    auto &next = getNextPeer(target, hopId);
                    if (next.id == localPeer.id) {
                        Packet response;
                        response.add(Opcode::LOOKUP_REPLY);
                        response.add(localPeer.id);
                        response.add(source);
                        response.add(replyHop);
                        response.add(localPeer.ep.getPort());
                        response.addStr(localPeer.ep.getAddress());
                        socket.write(response.data(), response.size(), getNextPeer(replyHop, localPeer.id).ep);
                    } else {
                        socket.write(buffer.data() + packetStartIndex, request.offset - packetStartIndex, next.ep);
                    }
                    break;
                }
                case LOOKUP_REPLY: {
                    PeerId destination = request.get<PeerId>();
                    PeerId replyHop = request.get<PeerId>();
                    uint16_t port = request.get<uint16_t>();
                    std::string address = request.getStr();

                    if (destination == localPeer.id) {
                        if (source != localPeer.id && !isInCache(source)) {
                            Endpoint hop(address.c_str(), port, true);
                            addToCache({source, hop});
                            Packet response;
                            response.add(Opcode::HANDSHAKE);
                            response.add(localPeer.id);
                            socket.write(response.data(), response.size(), hop);
                        }
                        break;
                    }

                    auto &next = getNextPeer(replyHop, hopId);
                    if (next.id == localPeer.id) {
                        socket.write(buffer.data() + packetStartIndex, request.offset - packetStartIndex,
                                     getNextPeer(destination, localPeer.id).ep);
                    } else {
                        socket.write(buffer.data() + packetStartIndex, request.offset - packetStartIndex, next.ep);
                    }
                    break;
                }
                case ROUTE: {
                    PeerId destination = request.get<PeerId>();
                    std::string msg = request.getStr();
                    auto &next = getNextPeer(destination, hopId);
                    if (next.id == localPeer.id) {
                        if (destination == localPeer.id) {
                            if (msgCallback) {
                                msgCallback(source, msg);
                            }
                        } else {
                            onError(Error("invalid route"));
                        }
                    } else {
                        socket.write(buffer.data() + packetStartIndex, request.offset - packetStartIndex, next.ep);
                    }
                    break;
                }
                case BROADCAST: {
                    PeerId oneTimeIdentifier = request.get<PeerId>();
                    std::string msg = request.getStr();

                    if (broadcasts.find(oneTimeIdentifier) != broadcasts.end()) {
                        break;
                    }
                    broadcasts[oneTimeIdentifier] = msg;

                    for (auto &peer : peerCache) {
                        socket.write(buffer.data() + packetStartIndex, request.offset - packetStartIndex, peer.ep);
                    }
                    if (msgCallback) {
                        msgCallback(source, msg);
                    }
                    break;
                }
                case DISCONNECT: {
                    bool wasInCache = false;
                    for (int i = 0; i < peerCache.size(); i++) {
                        if (peerCache[i].id == source) {
                            onInfo("disconnect: " + hex(source));
                            peerCache.erase(peerCache.begin() + i);
                            i--;
                            wasInCache = true;
                        }
                    }
                    if (wasInCache) {
                        //fill empty distance slot for disconnected peer
                        PeerId dist = source ^ localPeer.id;
                        int level = -1;
                        while (dist >= (PeerId) 1) {
                            dist = dist >> 1;
                            level++;
                        }
                        dist = (PeerId) 1 << level;
                        PeerId target = localPeer.id ^dist;

                        Packet response;
                        response.add(Opcode::LOOKUP);
                        response.add(localPeer.id);
                        response.add(target);
                        auto &hop = getNextPeer(target, localPeer.id);
                        response.add(hop.id);
                        socket.write(response.data(), response.size(), hop.ep);
                    }
                    break;
                }
                default:
                    onError(Error("invalid opcode"));
            }
        }
    }

    void PeerNetwork::addToCache(const Peer &peer) {
        bool has = false;
        for (auto &p : peerCache) {
            if (p.id == peer.id) {
                has = true;
                break;
            }
        }
        if (!has) {
            onInfo("peer: " + hex(peer.id) + " " + peer.ep.getAddress() + " " + std::to_string(peer.ep.getPort()));
            peerCache.push_back(peer);
        }
    }

    bool PeerNetwork::isInCache(PeerId id) {
        for (int i = 0; i < peerCache.size(); i++) {
            if (peerCache[i].id == id) {
                return true;
            }
        }
        return false;
    }

    const char *PeerNetwork::opcodeName(PeerNetwork::Opcode opcode) {
        switch (opcode) {
            case NONE:
                return "NONE";
            case PING:
                return "PING";
            case PONG:
                return "PONG";
            case HANDSHAKE:
                return "HANDSHAKE";
            case HANDSHAKE_REPLY:
                return "HANDSHAKE_REPLY";
            case LOOKUP:
                return "LOOKUP";
            case LOOKUP_REPLY:
                return "LOOKUP_REPLY";
            case ROUTE:
                return "ROUTE";
            case BROADCAST:
                return "BROADCAST";
            case DISCONNECT:
                return "DISCONNECT";
            default:
                return "INVALID";
        }
    }

    void PeerNetwork::broadcast(const std::string &msg) {
        Packet packet;
        packet.add(Opcode::BROADCAST);
        packet.add(localPeer.id);

        PeerId oneTimeIdentifier;
        for (int i = 0; i < sizeof(localPeer.id); i++) {
            oneTimeIdentifier.data[i] = (unsigned char) std::rand();
        }
        packet.add(oneTimeIdentifier);
        broadcasts[oneTimeIdentifier] = msg;

        packet.addStr(msg);
        for (auto &peer : peerCache) {
            socket.write(packet.data(), packet.size(), peer.ep);
        }
    }

    void PeerNetwork::stop() {
        running = false;
        handler.stop();
    }

    std::string PeerNetwork::hex(PeerId id, bool shortVersion) {
        std::string str;
        for (int i = sizeof(id) - 1; i >= 0; i--) {
            int value = id.data[i];
            str += "0123456789abcdef"[value / 16];
            str += "0123456789abcdef"[value % 16];
            if (shortVersion && i <= sizeof(id) - 2) {
                break;
            }
        }
        return str;
    }

    const Peer &PeerNetwork::getNextPeer(PeerId destination, PeerId except) {
        PeerId minimalDistance = localPeer.id ^destination;
        int minimalIndex = -1;

        if (localPeer.id == except) {
            minimalIndex = -2;
        }

        for (int i = 0; i < peerCache.size(); i++) {
            if (peerCache[i].id != except) {
                PeerId distance = peerCache[i].id ^destination;
                if (distance < minimalDistance || minimalIndex == -2) {
                    minimalDistance = distance;
                    minimalIndex = i;
                }
            }
        }

        if (minimalIndex < 0) {
            return localPeer;
        } else {
            return peerCache[minimalIndex];
        }
    }

    void PeerNetwork::onError(Error error) {
        if (logCallback) {
            logCallback(2, "error: " + std::to_string(error.code) + " " + error.message);
        }
    }

    void PeerNetwork::onInfo(const std::string &msg) {
        if (logCallback) {
            logCallback(1, msg);
        }
    }

    void PeerNetwork::onDebug(const std::string &msg) {
        if (logCallback) {
            logCallback(0, msg);
        }
    }

}
