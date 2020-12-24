//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#include "pnet/peer/PeerNetwork.h"
#include "pnet/Terminal.h"
#include "pnet/util.h"
#include <iostream>
#include <csignal>

using namespace pnet;

Terminal terminal;
PeerNetwork net;

void onExit(int sig){
    terminal.stop();
    net.disconnect();
    net.stop();
}

void printInfo(){
    terminal.print(str("local:\n", hex(net.localId()), " ", net.getPeers()[0].ep.getAddress(), " ", net.getPeers()[0].ep.getPort()));
    terminal.print(str(net.getPeers().size() - 1, " peer(s):"));
    for(int i = 1; i < net.getPeers().size(); i++){
        auto &peer = net.getPeers()[i];
        terminal.print(str(hex(peer.id), " ", peer.ep.getAddress(), " ", peer.ep.getPort()));
    }
}

int main(int argc, char *argv[]){
    std::string ip = "::1";
    int port = 2000;

    std::string entryId = "::1";
    int entryPort = 2000;

    int logLevel = 2;

    //parse arguments
    for(int i = 1; i < argc; i++){
        std::string arg = argv[i];
        if(arg == "-v"){
            try{
                if(argc > i+1){
                    logLevel = std::stoi(argv[i+1]);
                }
            } catch (...) {}
            i += 1;
            continue;
        }else if(arg == "-e"){
            if(argc > i+1){
                entryId = argv[i+1];
            }
            try{
                if(argc > i+2){
                    entryPort = std::stoi(argv[i+2]);
                }
            } catch (...) {}
            i += 2;
            continue;
        }else if(arg == "-l"){
            if(argc > i+1){
                ip = argv[i+1];
            }
            try{
                if(argc > i+2){
                    port = std::stoi(argv[i+2]);
                }
            } catch (...) {}
            i += 2;
            continue;
        }
    }

    //set logging callback
    net.logCallback = [&](int level, const std::string &msg){
        if(level >= logLevel){
            terminal.print(msg);
        }
    };

    //listen on first free port
    Error error;
    for(int i = 0; i < 1024; i++){
        error = net.start(port, ip.c_str());
        if(!error){
            break;
        }
        port++;
    }
    if(error){
        std::cout << error.message << std::endl;
    }

    //add entry nodes
    for(int i = 0; i < 10; i++){
        net.addEntryNode(Endpoint(entryId.c_str(), entryPort + i));
    }

    net.msgCallback = [&](PeerId id, const std::string &msg){
        if(msg == "shutdown"){
            terminal.stop();
            net.disconnect();
            net.stop();
        }else if(msg == "info all"){
            printInfo();
        }else{
            terminal.print(str(hex(id, true), ": ", msg));
        }
    };


    net.join();

    terminal.run(hex(net.localId(), true) + "\033[32m>\033[0m ", [&](const std::string &msg){
        if(msg == "exit"){
            terminal.stop();
            net.disconnect();
            net.stop();
        }else if(msg == "shutdown") {
            net.broadcast(msg);
            terminal.stop();
            net.disconnect();
            net.stop();
        }else if(msg == "info") {
            printInfo();
        }else if(msg == "info all") {
            printInfo();
            net.broadcast(msg);
        }else{
            if(msg[0] == '@'){
                int i = 0;
                PeerId destination = 0;
                for(i = 1; i < msg.size(); i++){
                    if(msg[i] == ' '){
                        i++;
                        break;
                    }else{
                        char c = msg[i];
                        if(c >= '0' && c <= '9'){
                            destination.data[sizeof(PeerId) - 1 - (i-1) / 2] |= (c - '0') << ((i-1) % 2 == 0 ? 4 : 0);
                        }else if(c >= 'a' && c <= 'f'){
                            destination.data[sizeof(PeerId) - 1 - (i-1) / 2] |= (c - 'a' + 10) << ((i-1) % 2 == 0 ? 4 : 0);
                        }
                    }
                }
                net.send(msg.substr(i), destination);
            }else{
                net.broadcast(msg);
            }
        }
    });
    signal(SIGINT, onExit);

    net.waitForStop();
    return 0;
}
