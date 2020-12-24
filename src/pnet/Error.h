//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#ifndef SOCKET_ERROR_H
#define SOCKET_ERROR_H

namespace pnet {

    enum ErrorCode{
        OK,
        ERROR,
        DISCONNECT,
        TIMEOUT,
        BIND_FAIL
    };

    class Error {
    public:
        ErrorCode code;
        const char *message;

        Error(ErrorCode code = OK, const char *message = "");
        Error(const char *message, ErrorCode code = ERROR);
        operator bool() const;
        bool operator==(const Error &error) const;
        bool operator!=(const Error &error) const;
        bool operator==(const ErrorCode &code) const;
        bool operator!=(const ErrorCode &code) const;
    };

}

#endif //SOCKET_ERROR_H
