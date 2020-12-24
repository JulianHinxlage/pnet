//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#ifndef SOCKET_ENDPOINT_H
#define SOCKET_ENDPOINT_H

#include <memory>

namespace pnet {

    class Endpoint {
    public:
        Endpoint();
        Endpoint(const char *address, uint16_t port, bool resolve = false);
        Endpoint(const Endpoint &ep);
        Endpoint &operator=(const Endpoint &ep);
        bool operator==(const Endpoint &ep) const;
        bool operator!=(const Endpoint &ep) const;

        uint16_t getPort() const;
        void setPort(uint16_t port);
        const char *getAddress() const;
        void setAddress(const char *address, bool resolve = false);
        void set(const char *address, uint16_t port, bool resolve = false);
        bool valid() const;
        bool isv4() const;
        bool isv6() const;
        void *getHandle() const;
    private:
        class Impl;
        std::shared_ptr<Impl> impl;
    };

}

#endif //SOCKET_ENDPOINT_H
