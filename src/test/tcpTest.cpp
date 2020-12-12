//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#include "socket/TcpListener.h"
#include "socket/SocketHandler.h"
#include <iostream>

using namespace sock;

int main(int argc, char *argv[]){

    //parse arguments
    bool serverFlag = false;
    const char *address = "127.0.0.1";
    uint16_t port = 1234;
    int pos = 0;
    for(int i = 1; i < argc; i++){
        std::string arg = argv[i];
        if(arg == "-s"){
            serverFlag = true;
        }else{
            if(pos == 1){
                address = argv[i];
            }else if (pos == 0){
                try{
                    port = std::stoi(arg);
                } catch (...) {}
            }
            pos++;
        }
    }


    if(serverFlag){
        //server
        SocketHandler handler;
        TcpListener listener;
        Error error;
        error = listener.listen(port);
        if(error){
            std::cout << error.message << std::endl;
            return 0;
        }

        std::vector<TcpSocket> sockets;

        handler.add(listener.getHandle(), [&](){
            TcpSocket socket;
            error = listener.accept(socket);
            if(error){
                std::cout << error.message << std::endl;
                return;
            }
            sockets.push_back(socket);
            handler.add(socket.getHandle(), [&, socket]() mutable{
               std::vector<char> buffer;
               int bytes;
               error = socket.readAll(buffer, bytes, 0);
                buffer[bytes] = '\0';
                std::string msg = buffer.data();

                if(msg == "shutdown"){
                    socket.disconnect();
                    handler.stop();
                }

                if(error){
                    if(error == DISCONNECT){
                        handler.remove(socket.getHandle());
                        for(int i = 0; i < sockets.size(); i++){
                            if(sockets[i].getHandle() == socket.getHandle()){
                                sockets.erase(sockets.begin() + i);
                                i--;
                            }
                        }
                    }
                    std::cout << error.message << std::endl;
                    return;
                }

                std::cout << msg << std::endl;
                socket.write(msg.data(), msg.size());

            });
        });

        error = handler.run();
        if(error){
            std::cout << error.message << std::endl;
        }
    }else{
        //client
        TcpSocket socket;
        Error error;

        error = socket.connect(address, port, true);
        if(error){
            std::cout << error.message << std::endl;
            return -1;
        }

        std::string msg;
        while(msg != "exit"){
            std::getline(std::cin, msg, '\n');

            error = socket.write(msg.data(), msg.size());
            if(error){
                std::cout << error.message << std::endl;
                return -1;
            }

            if(msg == "shutdown"){
                socket.disconnect();
                break;
            }

            int bytes = 0;
            std::vector<char> buffer;
            error = socket.readAll(buffer, bytes);
            if(error){
                std::cout << error.message << std::endl;
                return -1;
            }
            msg = buffer.data();
            std::cout << msg << std::endl;
        }
    }

    return 0;
}
