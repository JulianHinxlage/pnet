//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#include "pnet/SocketHandler.h"
#include <cerrno>
#include <cstring>
#include <poll.h>

namespace pnet {

    class SocketHandler::Impl{
    public:
        std::vector<pollfd> pollSet;
        std::vector<std::function<void()>> onPoll;
        bool running;

        Impl(){
            running = false;
        }
    };

    SocketHandler::SocketHandler() {
        impl = std::make_shared<Impl>();
    }

    void SocketHandler::add(int handle, const std::function<void()> &callback) {
        pollfd poll;
        poll.fd = handle;
        poll.events = POLLIN;
        impl->pollSet.push_back(poll);
        impl->onPoll.push_back(callback);
    }

    void SocketHandler::remove(int handle) {
        for(int i = 0; i < impl->pollSet.size(); i++){
            if(impl->pollSet[i].fd == handle){
                std::swap(impl->pollSet[i], impl->pollSet.back());
                std::swap(impl->onPoll[i], impl->onPoll.back());
                impl->pollSet.pop_back();
                impl->onPoll.pop_back();
                i--;
            }
        }
    }

    Error SocketHandler::run(int timeoutMillis) {
        impl->running = true;
        while(impl->running){
            int code = ::poll(impl->pollSet.data(), impl->pollSet.size(), timeoutMillis);
            switch(code){
                case -1:
                    impl->running = false;
                    return Error(ErrorCode::ERROR, strerror(errno));
                case 0:
                    //timeout
                    break;
                default:
                    for(int i = 0; i < impl->pollSet.size(); i++){
                        auto &poll = impl->pollSet[i];
                        if(poll.revents & POLLIN){
                            impl->onPoll[i]();
                            break;
                        }
                    }
            }
        }
        return Error();
    }

    void SocketHandler::stop() {
        impl->running = false;
    }

}
