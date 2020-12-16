//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#include "socket/Terminal.h"

#include <termios.h>
#include <unistd.h>

namespace sock {

    static termios t_old;

    Terminal::~Terminal(){
        shutdown();
    }

    void Terminal::shutdown(){
        if(thread){
            if(thread->joinable()){
                thread->detach();
            }
        }
        thread = nullptr;
        tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    }

    void Terminal::print(const std::string &msg){
        std::cout << "\r";
        for(int i = 0; i < line.size(); i++){
            std::cout << " ";
        }
        std::cout << "\r";
        std::cout << msg << "\n";
        std::cout << line;
        std::cout.flush();
    }

    char Terminal::get(){
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

    void Terminal::run(const std::string &prefix, const std::function<void(const std::string &msg)> &callback){
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

    void Terminal::stop(){
        running = false;
    }

}