//
// Created by nugde on 25. 10. 8..
//

#pragma once

#include <utility>

template <typename Args>
class callable;

template <typename Return, typename... Args>
class callable<Return(Args...)> {
   public:
    virtual ~callable() = default;

   public:
    Return operator()(Args... args) {
        return this->run(std::forward<Args>(args)...);
    }

    virtual Return run(Args...) = 0;
};
