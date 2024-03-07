//
// Created by nugdev on 3/6/24.
//

#include <iostream>
#include <ini-parser/File.h>

int main() {
    // ConfigParser parser(L"config.ini");
    nugdev::ndk::ini::File file(L"config.ini");
    file.parse();

    std::wcout << file << std::endl;

    file.save(L"./test");

    return 0;
}
