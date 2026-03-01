#include "kern/support/Arena.h"
#include <algorithm>
#include <cstdlib>

namespace kern {

Arena::~Arena() {
    for (char* block : blocks_) {
        std::free(block);
    }
}

void Arena::newBlock(size_t min_size) {
    size_t size = std::max(BLOCK_SIZE, min_size);
    char* block = static_cast<char*>(std::malloc(size));
    blocks_.push_back(block);
    ptr_ = block;
    end_ = block + size;
}

void* Arena::allocate(size_t size, size_t align) {
    // Align ptr_
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr_);
    uintptr_t aligned = (addr + align - 1) & ~(align - 1);
    char* result = reinterpret_cast<char*>(aligned);

    if (result + size > end_) {
        newBlock(size + align);
        addr = reinterpret_cast<uintptr_t>(ptr_);
        aligned = (addr + align - 1) & ~(align - 1);
        result = reinterpret_cast<char*>(aligned);
    }

    ptr_ = result + size;
    return result;
}

} // namespace kern
