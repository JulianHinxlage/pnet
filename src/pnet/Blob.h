//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#ifndef SOCKET_BLOB_H
#define SOCKET_BLOB_H

namespace pnet {

    //blob of memory with logic and bitwise operators
    template<int bytes>
    class Blob {
    public:
        unsigned char data[bytes];

        Blob(){
            for(int i = 0; i < bytes; i++){
                data[i] = 0;
            }
        }

        Blob(const Blob &blob){
            operator=(blob);
        }

        template<typename T>
        Blob(const T &t){
            operator=(t);
        }

        Blob &operator=(const Blob &blob){
            for(int i = 0; i < bytes; i++){
                data[i] = blob.data[i];
            }
            return *this;
        }

        template<typename T>
        Blob &operator=(const T &t){
            int size = bytes < (int)sizeof(t) ? bytes : (int)sizeof(t);
            for(int i = 0; i < size; i++){
                data[i] = ((char*)&t)[i];
            }for(int i = size; i < bytes; i++){
                data[i] = 0;
            }
            return *this;
        }

        bool operator==(const Blob &blob) const{
            for(int i = 0; i < bytes; i++){
                if(data[i] != blob.data[i]){
                    return false;
                }
            }
            return true;
        }

        bool operator!=(const Blob &blob) const{
            return !operator==(blob);
        }

        bool operator<(const Blob &blob) const{
            for(int i = bytes-1; i >= 0; i--){
                if(data[i] < blob.data[i]){
                    return true;
                }else if(data[i] > blob.data[i]){
                    return false;
                }
            }
            return false;
        }

        bool operator>(const Blob &blob) const{
            for(int i = bytes-1; i >= 0; i--){
                if(data[i] > blob.data[i]){
                    return true;
                }else if(data[i] < blob.data[i]){
                    return false;
                }
            }
            return false;
        }

        bool operator<=(const Blob &blob) const{
            return !operator>(blob);
        }

        bool operator>=(const Blob &blob) const{
            return !operator<(blob);
        }

        Blob operator&(const Blob &blob) const{
            Blob result;
            for(int i = 0; i < bytes; i++){
                result.data[i] = data[i] & blob.data[i];
            }
            return result;
        }

        Blob operator|(const Blob &blob) const{
            Blob result;
            for(int i = 0; i < bytes; i++){
                result.data[i] = data[i] | blob.data[i];
            }
            return result;
        }

        Blob operator^(const Blob &blob) const{
            Blob result;
            for(int i = 0; i < bytes; i++){
                result.data[i] = data[i] ^ blob.data[i];
            }
            return result;
        }

        Blob operator~() const{
            Blob result;
            for(int i = 0; i < bytes; i++){
                result.data[i] = ~data[i];
            }
            return result;
        }

        Blob &operator&=(const Blob &blob){
            *this = *this & blob;
            return *this;
        }

        Blob &operator|=(const Blob &blob){
            *this = *this | blob;
            return *this;
        }

        Blob &operator^=(const Blob &blob){
            *this = *this ^ blob;
            return *this;
        }

        Blob operator<<(int shift){
            Blob result;
            int byteShift = shift / 8;
            int bitShift = shift % 8;
            for(int i = 0; i < bytes; i++){
                if(i - byteShift >= 0){
                    if(bitShift == 0){
                        result.data[i] = data[i - byteShift];
                    }else{
                        result.data[i] = (data[i - byteShift] << bitShift);
                        if(i - byteShift - 1 >= 0){
                            result.data[i] |= (data[i - byteShift - 1] >> (8 - bitShift));
                        }
                    }
                }
            }
            return result;
        }

        Blob operator>>(int shift){
            Blob result;
            int byteShift = shift / 8;
            int bitShift = shift % 8;
            for(int i = 0; i < bytes; i++){
                if(i + byteShift < bytes){
                    if(bitShift == 0){
                        result.data[i] = data[i + byteShift];
                    }else{
                        result.data[i] = (data[i + byteShift] >> bitShift);
                        if(i + byteShift + 1 < bytes){
                            result.data[i] |= (data[i + byteShift + 1] << (8 - bitShift));
                        }
                    }
                }
            }
            return result;
        }
    };

}

#endif //SOCKET_BLOB_H
