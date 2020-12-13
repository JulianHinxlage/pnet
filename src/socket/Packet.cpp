//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#include "Packet.h"

namespace sock {

    Packet::Packet() {
        bytes = 0;
        offset = 0;
    }

    Packet::Packet(const std::vector<char> &buffer, int bytes) {
        this->buffer = buffer;
        this->bytes = bytes;
        this->offset = 0;
    }

    std::string Packet::remaining() {
        return std::string(buffer.data() + offset, bytes - offset);
    }

    char *Packet::data() {
        return buffer.data() + offset;
    }

    int Packet::size() {
        return bytes - offset;
    }

    std::string Packet::getStr(){
        std::string str;
        while(offset < bytes){
            if(buffer[offset] == '\0'){
                offset++;
                break;
            }else{
                str.push_back(buffer[offset++]);
            }
        }
        return str;
    }

    void Packet::addStr(const std::string &str){
        for(int i = 0; i < str.size(); i++){
            if(buffer.size() > bytes){
                buffer[bytes++] = str[i];
            }else{
                buffer.push_back(str[i]);
                bytes++;
            }
        }
        if(buffer.size() > bytes){
            buffer[bytes++] = '\0';
        }else{
            buffer.push_back('\0');
            bytes++;
        }
    }

}
