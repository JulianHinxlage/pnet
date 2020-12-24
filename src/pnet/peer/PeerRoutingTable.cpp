//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#include "PeerRoutingTable.h"

namespace pnet {

    std::string hex(PeerId id, bool shortVersion) {
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

    PeerRoutingTable::PeerRoutingTable() {
        peers.push_back({(PeerId)0, Endpoint()});
    }

    Peer &PeerRoutingTable::localPeer() {
        return peers[0];
    }

    void PeerRoutingTable::add(const Peer &peer) {
        if(!has(peer.id)){
            peers.push_back(peer);
        }
    }

    void PeerRoutingTable::add(const PeerId &id, const Endpoint &ep) {
        if(!has(id)){
            peers.push_back({id, ep});
        }
    }

    bool PeerRoutingTable::has(const PeerId &id) {
        for(auto &peer : peers){
            if(peer.id == id){
                return true;
            }
        }
        return false;
    }

    bool PeerRoutingTable::has(const Endpoint &ep) {
        for(auto &peer : peers){
            if(peer.ep == ep){
                return true;
            }
        }
        return false;
    }

    const Peer &PeerRoutingTable::get(const PeerId &id) {
        for(auto &peer : peers){
            if(peer.id == id){
                return peer;
            }
        }
        return defaultPeer;
    }

    const Peer &PeerRoutingTable::get(const Endpoint &ep) {
        for(auto &peer : peers){
            if(peer.ep == ep){
                return peer;
            }
        }
        return defaultPeer;
    }

    const Peer &PeerRoutingTable::getNext(const PeerId &id, const PeerId &except) {
        int minIndex = -1;
        PeerId minDistance = 0;

        for(int i = 0; i < peers.size(); i++){
            if(peers[i].id != except) {
                PeerId distance = id ^peers[i].id;
                if (minIndex == -1 || distance < minDistance) {
                    minDistance = distance;
                    minIndex = i;
                }
            }
        }

        if(minIndex != -1){
            return peers[minIndex];
        }else{
            return defaultPeer;
        }
    }

    PeerId PeerRoutingTable::lookupTarget(int level) {
        return localPeer().id ^ (PeerId(1) << level);
    }

    bool PeerRoutingTable::remove(const PeerId &id) {
        bool removed = false;
        for(int i = 1; i < peers.size(); i++){
            if(peers[i].id == id){
                peers.erase(peers.begin() + i);
                i--;
                removed = true;
            }
        }
        return removed;
    }

    int PeerRoutingTable::getLevel(PeerId id) {
        PeerId dist = id ^ localPeer().id;
        int level = -1;
        while (dist > (PeerId) 0) {
            dist = dist >> 1;
            level++;
        }
        return level;
    }

}