//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#ifndef SOCKET_TERMINAL_H
#define SOCKET_TERMINAL_H

#include <iostream>
#include <thread>
#include <functional>

namespace pnet {

    class Terminal{
    public:
        std::string line;
        std::shared_ptr<std::thread> thread;
        bool running;

        ~Terminal();
        void shutdown();
        void print(const std::string &msg);
        char get();
        void run(const std::string &prefix, const std::function<void(const std::string &msg)> &callback);
        void stop();
    };

}

#endif //SOCKET_TERMINAL_H
