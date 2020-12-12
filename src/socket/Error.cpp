//
// Copyright (c) 2020 Julian Hinxlage. All rights reserved.
//

#include "Error.h"

namespace socket {

    Error::Error(ErrorCode code, const char *message)
        : code(code), message(message) {}

    Error::Error(const char *message, ErrorCode code)
        : code(code), message(message) {}

    Error::operator bool() const {
        return (bool)code;
    }

    bool Error::operator==(const Error &error) const {
        return code == error.code;
    }

    bool Error::operator!=(const Error &error) const {
        return code != error.code;
    }

    bool Error::operator==(const ErrorCode &code) const {
        return this->code == code;
    }

    bool Error::operator!=(const ErrorCode &code) const {
        return this->code != code;
    }

}
