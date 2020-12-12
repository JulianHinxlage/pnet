//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#include "socket/UdpSocket.h"
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
        UdpSocket socket;
        Error error;
        error = socket.listen(port);
        if(error){
            std::cout << error.message << std::endl;
            return 0;
        }

        while(true) {
            int bytes = 0;
            std::vector<char> buffer;
            Endpoint source;
            error = socket.readAll(buffer, bytes, source);
            if (error) {
                std::cout << error.message << std::endl;
                break;
            }
            buffer[bytes] = '\0';
            std::string msg = buffer.data();


            if (msg == "shutdown") {
                break;
            }

            std::cout << msg << std::endl;
            socket.write(msg.data(), msg.size(), source);
        }


    }else{
        //client
        UdpSocket socket;
        Error error;
        Endpoint destination(address, port, true);

        std::string msg;
        while(msg != "exit"){
            std::getline(std::cin, msg, '\n');

            error = socket.write(msg.data(), msg.size(), destination);
            if(error){
                std::cout << error.message << std::endl;
                return -1;
            }

            if(msg == "shutdown"){
                break;
            }

            int bytes = 0;
            std::vector<char> buffer;
            error = socket.readAll(buffer, bytes, destination);
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
