#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <new>
#include <utility>

namespace kern {

class Arena {
public:
    static constexpr size_t BLOCK_SIZE = 4096;

    Arena() = default;
    ~Arena();

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&& other) noexcept
        : blocks_(std::move(other.blocks_)), ptr_(other.ptr_), end_(other.end_) {
        other.ptr_ = nullptr;
        other.end_ = nullptr;
    }
    Arena& operator=(Arena&&) = delete;

    template <typename T, typename... Args>
    T* make(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        return new (mem) T(std::forward<Args>(args)...);
    }

    template <typename T>
    T* makeArray(size_t count) {
        void* mem = allocate(sizeof(T) * count, alignof(T));
        for (size_t i = 0; i < count; ++i) {
            new (static_cast<char*>(mem) + sizeof(T) * i) T();
        }
        return static_cast<T*>(mem);
    }

    void* allocate(size_t size, size_t align);

private:
    void newBlock(size_t min_size);

    std::vector<char*> blocks_;
    char* ptr_ = nullptr;
    char* end_ = nullptr;
};

} // namespace kern
