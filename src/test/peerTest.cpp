//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#include "socket/PeerNetwork.h"
#include <iostream>
#include <thread>

#include <termios.h>
#include <unistd.h>
#include <signal.h>

using namespace sock;

class Terminal{
public:
    std::string line;
    std::shared_ptr<std::thread> thread;
    bool running;
    termios t_old;

    ~Terminal(){
        shutdown();
    }

    void shutdown(){
        if(thread){
            thread->detach();
        }
        thread = nullptr;
        tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    }

    void print(const std::string &msg){
        std::cout << "\r";
        for(int i = 0; i < line.size(); i++){
            std::cout << " ";
        }
        std::cout << "\r";
        std::cout << msg << "\n";
        std::cout << line;
        std::cout.flush();
    }

    char get(){
        int ch;
        struct termios t_new;

        tcgetattr(STDIN_FILENO, &t_old);
        t_new = t_old;
        t_new.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

        ch = getchar();

        tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
        return ch;
    }

    void run(const std::string &prefix, const std::function<void(const std::string &msg)> &callback){
        running = true;
        thread = std::make_shared<std::thread>([&, callback, prefix](){
            line = prefix;
            std::cout << line;
            std::cout.flush();

            while(running) {
                char c = get();
                if (c == '\n') {
                    std::cout << "\r";
                    std::cout << line << "\n";
                    if(line.size() > prefix.size()){
                        std::string msg = line.substr(prefix.size());
                        line.clear();
                        callback(msg);
                    }else{
                        line.clear();
                    }
                    line = prefix;
                    if(running){
                        std::cout << line;
                        std::cout.flush();
                    }
                } else {
                    if(c == 127){//backspace
                        line.pop_back();
                        std::cout << "\b \b";
                    }else{
                        line.push_back(c);
                    }
                    std::cout << "\r" << line;
                    std::cout.flush();
                }
            }
        });
    }

    void stop(){
        running = false;
    }
};
Terminal terminal;
PeerNetwork net;

void onExit(int sig){
    terminal.stop();
    terminal.shutdown();
    net.stop();
}

int main(int argc, char *argv[]){
    int port = 2000;
    bool debug = false;
    for(int i = 1; i < argc; i++){
        if(std::string(argv[i]) == "-d"){
            debug = true;
            continue;
        }
        try{
            port = std::stoi(argv[i]);
        } catch (...) {}
    }

    net.port = port;
    for(int i = 0; i < 10; i++){
        net.entryNodes.push_back(Endpoint("127.0.0.1", 2000 + i));
    }

    net.msgCallback = [&](PeerId id, const std::string &msg){
        if(msg == "shutdown"){
            terminal.stop();
            net.stop();
        }
        terminal.print(net.hex(id, true) + ": " + msg);
    };
    net.logCallback = [&](int level, const std::string &msg){
        if(level >= (int)!debug){
            terminal.print(msg);
        }
    };

    terminal.run("> ", [&](const std::string &msg){
        if(msg == "exit"){
            terminal.stop();
            net.stop();
        }else if(msg == "shutdown") {
            net.broadcast(msg);
            terminal.stop();
            net.stop();
        }else if(msg == "info") {
            terminal.print(std::to_string(net.peerCache.size()) + " peer(s):");
            for(auto &peer : net.peerCache){
                terminal.print(net.hex(peer.id) + " " + peer.ep.getAddress() + " " + std::to_string(peer.ep.getPort()));
            }
        }else{
            net.broadcast(msg);
        }
    });
    signal(SIGINT, onExit);

    Error error;
    for(int i = 0; i < 10; i++){
        error = net.run();
        if(error != BIND_FAIL){
            break;
        }
        net.port++;
    }
    if(error){
        std::cout << error.message << std::endl;
    }
    return 0;
}
