//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#include "PeerNetwork.h"
#include "socket/util.h"
#include <random>
#include <unordered_map>

namespace sock {

    template<int bytes>
    Blob<bytes> randomId(){
        static bool seeded = false;
        if(!seeded){
            std::srand(std::random_device()());
            seeded = true;
        }
        Blob<bytes> id;
        for(int i = 0; i < sizeof(id); i++){
            id.data[i] = std::rand() & 0xff;
        }
        return id;
    }

    const char *opcodeName(PeerNetwork::Opcode opcode) {
        switch (opcode) {
            case PeerNetwork::NONE:
                return "NONE";
            case PeerNetwork::PING:
                return "PING";
            case PeerNetwork::PONG:
                return "PONG";
            case PeerNetwork::HANDSHAKE:
                return "HANDSHAKE";
            case PeerNetwork::HANDSHAKE_REPLY:
                return "HANDSHAKE_REPLY";
            case PeerNetwork::LOOKUP:
                return "LOOKUP";
            case PeerNetwork::LOOKUP_REPLY:
                return "LOOKUP_REPLY";
            case PeerNetwork::ROUTE:
                return "ROUTE";
            case PeerNetwork::BROADCAST:
                return "BROADCAST";
            case PeerNetwork::MESSAGE:
                return "MESSAGE";
            case PeerNetwork::DISCONNECT:
                return "DISCONNECT";
            default:
                return "INVALID";
        }
    }

    PeerNetwork::PeerNetwork() {
        thread = nullptr;
        readBuffer.resize(1024);
    }

    Error PeerNetwork::start(uint16_t port, const char *address) {
        //set local peer id
        routingTable.localPeer().id = randomId<sizeof(PeerId)>();
        routingTable.localPeer().ep.set(address, port, true);

        //listen on port
        Error error = socket.listen(port);
        if(error){
            return error;
        }

        //set packet processing callback
        handler.add(socket.getHandle(), [&](){
            readPacket(0);
        });

        //start handler and read packets
        thread = std::make_shared<std::thread>([&](){
            handler.run();
        });

        log(str("port: ", port), true);
        log(str("id: ", hex(localId())), true);

        return Error();
    }

    void PeerNetwork::stop() {
        handler.stop();
        handler.remove(socket.getHandle());
    }

    void PeerNetwork::readPacket(int millisTimeout) {
        int bytes = 0;
        Endpoint source;
        Error error = socket.readAll(readBuffer, bytes, source, millisTimeout);
        if(error){
            if(error == TIMEOUT){
                return;
            }else{
                logError(error);
                return;
            }
        }
        Packet packet(readBuffer, bytes);
        processPacket(packet, source);
    }

    void PeerNetwork::processPacket(Packet &packet, const Endpoint &sourceEp) {
        Peer hop;
        if(routingTable.has(sourceEp)){
            hop = routingTable.get(sourceEp);
        }else{
            hop.ep = sourceEp;
            hop.id = 0;
        }

        PeerId source = hop.id;
        PeerId destination = localId();

        while(packet.size() > 0){
            int packetStart = packet.offset;
            Opcode opcode = packet.get<Opcode>();

            log(str("[", opcodeName(opcode), "] ", hex(source, true), " ", hop.ep.getAddress(), " ", hop.ep.getPort()), true);

            switch (opcode) {
                case NONE:
                    break;
                case PING:{
                    Packet response;
                    response.add(PONG);
                    sendPacket(response, source);
                    break;
                }
                case PONG:
                    break;
                case HANDSHAKE:{
                    PeerId id = packet.get<PeerId>();
                    if(!routingTable.has(id)){
                        log(str("connect: ", hex(id, false)), false);
                        routingTable.add(id, hop.ep);
                    }
                    hop.id = id;
                    source = hop.id;

                    Packet response;
                    response.add(HANDSHAKE_REPLY);
                    response.add(localId());
                    sendPacket(response, source);
                    break;
                }
                case HANDSHAKE_REPLY:{
                    PeerId id = packet.get<PeerId>();
                    if(!routingTable.has(id)){
                        log(str("connect: ", hex(id, false)), false);
                        routingTable.add(id, hop.ep);
                    }
                    hop.id = id;
                    source = hop.id;
                    break;
                }
                case LOOKUP:{
                    PeerId relayId = packet.get<PeerId>();
                    Packet response;
                    response.add(LOOKUP_REPLY);
                    response.add(localId());
                    response.add(routingTable.localPeer().ep.getPort());
                    response.addStr(routingTable.localPeer().ep.getAddress());

                    Packet route;
                    route.add(ROUTE);
                    route.add(localId());
                    route.add(source);
                    route.add((int)response.size());
                    route.add(response.data(), response.size());

                    Packet route2;
                    route2.add(ROUTE);
                    route2.add(localId());
                    route2.add(relayId);
                    route2.add((int) route.size());
                    route2.add(route.data(), route.size());
                    auto &next = routingTable.getNext(relayId, localId());
                    socket.write(route2.data(), route2.size(), next.ep);
                    break;
                }
                case LOOKUP_REPLY:{
                    PeerId id = packet.get<PeerId>();
                    uint16_t port = packet.get<uint16_t>();
                    std::string address = packet.getStr();

                    Endpoint ep(address.c_str(), port, true);
                    if(!routingTable.has(id)) {
                        log(str("connect: ", hex(id, false)), false);
                        routingTable.add(id, ep);

                        Packet response;
                        response.add(HANDSHAKE);
                        response.add(localId());
                        socket.write(response.data(), response.size(), ep);
                    }
                    break;
                }
                case ROUTE:{
                    source = packet.get<PeerId>();
                    destination = packet.get<PeerId>();
                    int payloadSize = packet.get<int>();
                    auto &next = routingTable.getNext(destination, hop.id);
                    if(next.id != localId()){
                        socket.write(&packet.buffer[packetStart], packet.offset - packetStart + payloadSize, next.ep);
                        packet.skip(payloadSize);
                        source = hop.id;
                        destination = localId();
                    }
                    break;
                }
                case BROADCAST:{
                    PeerId broadcastSource = packet.get<PeerId>();
                    Blob<32> broadcastId = packet.get<Blob<32>>();
                    std::string msg = packet.getStr();
                    if(broadcastIds.find(broadcastId) == broadcastIds.end()){
                        broadcastIds[broadcastId] = true;
                        for(auto &peer : routingTable.peers){
                            if(peer.ep != hop.ep && peer.id != localId()){
                                if((broadcastSource ^ peer.id) > (broadcastSource ^ localId())){
                                    socket.write(&packet.buffer[packetStart], packet.offset - packetStart, peer.ep);
                                }
                            }
                        }
                        if(msgCallback){
                            msgCallback(broadcastSource, msg);
                        }
                    }
                    break;
                }
                case MESSAGE:{
                    std::string msg = packet.getStr();
                    if(destination.data[sizeof(PeerId)-1] == localId().data[sizeof(PeerId)-1]){
                        if(destination.data[sizeof(PeerId)-2] == localId().data[sizeof(PeerId)-2]){
                            if(msgCallback){
                                msgCallback(source, msg);
                            }
                        }
                    }
                    break;
                }
                case DISCONNECT:{
                    if(source == hop.id) {
                        if (routingTable.remove(source)) {
                            log(str("disconnect: ", hex(source, false)), false);
                            lookup(routingTable.lookupTarget(routingTable.getLevel(source)));
                        }
                        break;
                    }
                }
                default:
                    log("invalid opcode");
            }

            if(opcode != ROUTE){
                source = hop.id;
                destination = localId();
            }
        }
    }

    void PeerNetwork::lookup(const PeerId &target) {
        auto &next = routingTable.getNext(target, localId());
        Packet packet;
        packet.add(LOOKUP);
        packet.add(next.id);//relayId for routing back through the first hop
        sendPacket(packet, target);
    }

    void PeerNetwork::sendPacket(Packet &packet, const PeerId &destination) {
        auto &next = routingTable.getNext(destination, localId());
        if(next.id == destination){
            socket.write(packet.data(), packet.size(), next.ep);
        }else{
            Packet route;
            route.add(ROUTE);
            route.add(localId());
            route.add(destination);
            route.add((int)packet.size());
            route.add(packet.data(), packet.size());
            socket.write(route.data(), route.size(), next.ep);
        }
    }

    void PeerNetwork::broadcast(const std::string &msg){
        Blob<32> broadcastId = randomId<32>();

        Packet packet;
        packet.add(BROADCAST);
        packet.add(localId());
        packet.add(broadcastId);
        packet.addStr(msg);

        broadcastIds[broadcastId] = true;
        for(auto &peer : routingTable.peers){
            if(peer.id != localId()){
                socket.write(packet.data(), packet.size(), peer.ep);
            }
        }
    }

    void PeerNetwork::send(const std::string &msg, const PeerId &id) {
        Packet packet;
        packet.add(MESSAGE);
        packet.addStr(msg);
        sendPacket(packet, id);
    }

    Error PeerNetwork::join() {
        std::unordered_map<int, bool> map;
        for(int i = 0; i < entryNodes.size(); i++){
            int index = 0;
            do{
                index = std::rand() % entryNodes.size();
            }while(map.find(index) != map.end());
            map[index] = true;

            if(entryNodes[index] != routingTable.localPeer().ep) {
                Packet packet;
                packet.add(HANDSHAKE);
                packet.add(localId());

                socket.write(packet.data(), packet.size(), entryNodes[index]);

                log(str("try entry node: ", entryNodes[index].getAddress(), " ", entryNodes[index].getPort()), true);

                readPacket(200);
                if (isConnected()) {
                    break;
                }
            }
        }
        if(!isConnected()){
            return Error("could not find an entry node");
        }

        for(int level = sizeof(PeerId) * 8 - 1; level >= 0; level--){
            lookup(routingTable.lookupTarget(level));
        }

        return Error();
    }

    void PeerNetwork::disconnect() {
        Packet packet;
        packet.add(DISCONNECT);
        for(int i = 1; i < routingTable.peers.size(); i++){
            socket.write(packet.data(), packet.size(), routingTable.peers[i].ep);
        }
    }

    bool PeerNetwork::isConnected() {
        return routingTable.peers.size() > 1;
    }

    void PeerNetwork::logError(Error error) {
        if(logCallback){
            logCallback(2, str("error: [", error.code, "] ", error.message));
        }
    }

    void PeerNetwork::log(const std::string &msg, bool debug) {
        if(logCallback){
            logCallback(debug ? 0 : 1, msg);
        }
    }

    void PeerNetwork::addEntryNode(const Endpoint &ep) {
        entryNodes.push_back(ep);
    }

    PeerId PeerNetwork::localId() {
        return routingTable.localPeer().id;
    }

    void PeerNetwork::waitForStop() {
        if(thread){
            if(thread->joinable()){
                thread->join();
            }
        }
    }

    const std::vector<Peer> &PeerNetwork::getPeers() {
        return routingTable.peers;
    }

}
