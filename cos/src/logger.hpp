//
// Created by lambda on 10/25/25.
//

#pragma once

#include <format>
#include <iostream>
#include <string_view>

class Logger {
private:
    template<typename... Args>
    static std::string format_message(std::string_view format, Args &&... args) {
        return std::vformat(format, std::make_format_args(args...));
    }

public:
    template<typename... Args>
    inline static void trace(std::string_view format, Args &&... args) {
        std::cout << "[trace] " << format_message(format, std::forward<Args>(args)...) << '\n';
    }

    template<typename... Args>
    inline static void info(std::string_view format, Args &&... args) {
        std::cout << "[info] " << format_message(format, std::forward<Args>(args)...) << '\n';
    }

    template<typename... Args>
    inline static void warn(std::string_view format, Args &&... args) {
        std::cout << "[warn] " << format_message(format, std::forward<Args>(args)...) << '\n';
    }

    template<typename... Args>
    inline static void err(std::string_view format, Args &&... args) {
        std::cerr << "[err] " << format_message(format, std::forward<Args>(args)...) << '\n';
    }
};