//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#include "socket/TcpListener.h"
#include "socket/SocketHandler.h"
#include <iostream>

using namespace sock;

int main(int argc, char *argv[]){
    int port = 2000;
    for(int i = 1; i < argc; i++){
        try{
            port = std::stoi(argv[i]);
        } catch (...) {}
    }

    SocketHandler handler;
    TcpListener listener;
    Error error;


    error = listener.listen(port);
    if(error){
        std::cout << error.message << std::endl;
        return 0;
    }

    int request = 1;

    handler.add(listener.getHandle(), [&](){
        TcpSocket socket;
        error = listener.accept(socket);
        if(error){
            std::cout << error.message << std::endl;
            return;
        }
        handler.add(socket.getHandle(), [&, socket = socket]() mutable{
            std::vector<char> buffer;
            int bytes = 0;
            error = socket.readAll(buffer, bytes, 0);

            if(error){
                std::cout << error.message << std::endl;
                handler.remove(socket.getHandle());
                return;
            }

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
            handler.remove(socket.getHandle());
        });
    });

    handler.run();

    return 0;
}
