//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#include "pnet/TcpListener.h"
#include "pnet/SocketHandler.h"
#include <iostream>

using namespace pnet;

class Server{
public:
    TcpListener listener;
    SocketHandler handler;
    std::vector<TcpSocket> sockets;
    uint16_t  port;
    std::vector<char> buffer;
    std::function<void(TcpSocket &socket, char *ptr, int bytes)> readCallback;
    std::function<void(TcpSocket &socket)> disconnectCallback;
    std::function<void(TcpSocket &socket)> connectCallback;

    Server(uint16_t port){
        this->port = port;
    }

    void remove(int handle){
        handler.remove(handle);
        for(int i = 0; i < sockets.size(); i++){
            if(sockets[i].getHandle() == handle){
                sockets.erase(sockets.begin() + i);
                i--;
            }
        }
    }

    Error run(){
        Error error = listener.listen(port);
        if(error){
            return error;
        }

        handler.add(listener.getHandle(), [&](){
           TcpSocket socket;
           error = listener.accept(socket);
           if(!error){
               sockets.push_back(socket);
               if(connectCallback){
                   connectCallback(socket);
               }
               handler.add(socket.getHandle(), [&, socket]() mutable{
                   int bytes = 0;
                   error = socket.readAll(buffer, bytes, 0);
                   if(error){
                       if(error == DISCONNECT){
                           if(disconnectCallback){
                               disconnectCallback(socket);
                           }
                       }else{
                           std::cout << error.message << std::endl;
                       }
                       remove(socket.getHandle());
                   }else{
                       if(readCallback){
                           readCallback(socket, buffer.data(), bytes);
                       }
                   }
               });
           }
        });

        return handler.run();
    }
};

int main(int argc, char *argv[]){
    int port = 2000;
    for(int i = 1; i < argc; i++){
        try{
            port = std::stoi(argv[i]);
        } catch (...) {}
    }

    int request = 1;
    Server server(port);

    server.readCallback = [&](TcpSocket &socket, char *ptr, int bytes){
        std::string msg = R"(HTTP/1.0 200 OK
Server: Test
Content-Type: text/html

<!DOCTYPE html>
<html>
<head><title>Test</title></head>
<body>
<center><h1>Test</h1></center>
<hr><center>request )" + std::to_string(request++) + R"(</center>
</body>
</html>)";

        socket.write(msg.data(), msg.size());
        server.remove(socket.getHandle());
    };

    server.run();
    return 0;
}
