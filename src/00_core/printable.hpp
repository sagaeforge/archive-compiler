//
// Created by nugde on 25. 10. 8..
//

#pragma once

#include "common.h"

class printable {
public:
    virtual ~printable() = default;

    virtual void print(std::ostream &os) const = 0;

    friend std::ostream &operator<<(std::ostream &os, const printable &obj) {
        obj.print(os);
        return os;
    }

    friend std::ostream &operator<<(std::ostream &os, const printable *obj) {
        if (obj)
            obj->print(os);
        else
            os << "null";
        return os;
    }
};
