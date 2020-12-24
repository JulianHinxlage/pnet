//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#ifndef SOCKET_UTIL_H
#define SOCKET_UTIL_H

#include <sstream>

namespace pnet {

    template<typename... T>
    std::string str(const T &... t){
        std::stringstream ss;
        ((ss << t), ...);
        return ss.str();
    }

}

#endif //SOCKET_UTIL_H
