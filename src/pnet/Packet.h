//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#ifndef SOCKET_PACKET_H
#define SOCKET_PACKET_H

#include <vector>
#include <string>

namespace pnet {

    class Packet {
    public:
        std::vector<char> buffer;
        int bytes;
        int offset;

        Packet();
        Packet(const std::vector<char> &buffer, int bytes);
        std::string remaining();
        char *data();
        int size();
        std::string getStr();
        void addStr(const std::string &str);
        void add(char *ptr, int bytes);
        void skip(int bytes);

        template<typename T>
        T get(){
            T t;
            for(int i = 0; i < sizeof(t); i++){
                if(offset < bytes){
                    ((char*)&t)[i] = buffer[offset++];
                }
            }
            return t;
        }

        template<typename T>
        void add(const T &t){
            for(int i = 0; i < sizeof(t); i++){
                if(buffer.size() > bytes){
                    buffer[bytes++] = ((char*)&t)[i];
                }else{
                    buffer.push_back(((char*)&t)[i]);
                    bytes++;
                }
            }
        }

    };

}

#endif //SOCKET_PACKET_H
