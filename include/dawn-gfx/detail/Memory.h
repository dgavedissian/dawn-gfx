/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#pragma once

#include "../Base.h"
#include <cstddef>
#include <vector>
#include <memory>

namespace dw {
namespace gfx {
using MemoryDeleter = void (*)(std::byte*);

// A blob of memory.
class DW_API Memory {
public:
    /// Constructs an empty memory block.
    Memory();

    /// Create an uninitialised memory block.
    explicit Memory(usize size);

    /// Creates a memory block by copying from existing data.
    /// Size in bytes.
    template <typename T> Memory(const T* data, usize size);

    /// Creates a memory block by copying from existing data.
    template <typename T> explicit Memory(const std::vector<T>& data);

    /// Creates a memory block by moving from existing data (and taking ownership)
    template <typename T> explicit Memory(std::vector<T>&& data);

    /// Takes ownership of an existing memory block, deleting it using the provided deleter.
    /// Size in bytes.
    template <typename T, typename Deleter> Memory(T* data, usize size, Deleter deleter);

    /// Destroys memory block.
    ~Memory() = default;

    // Copyable.
    Memory(const Memory&) = default;
    Memory& operator=(const Memory&) = default;

    // Movable.
    Memory(Memory&&) = default;
    Memory& operator=(Memory&&) = default;

    /// Access individual bytes.
    std::byte& operator[](std::size_t index) const;

    /// Access underlying byte buffer.
    std::byte* data() const;

    /// Size of the byte buffer.
    usize size() const;

private:
    std::shared_ptr<std::byte> data_;
    usize size_;
    // A type erased pointer which owns an object that holds some data.
    std::shared_ptr<void> holder_;
};

template <typename T> Memory::Memory(const T* data, usize size) : Memory(data ? size : 0) {
    if (data != nullptr) {
        memcpy(data_.get(), data, size);
    }
}

template <typename T>
Memory::Memory(const std::vector<T>& data) : Memory(data.data(), data.size() * sizeof(T)) {
}

template <typename T> Memory::Memory(std::vector<T>&& data) {
    std::shared_ptr<std::vector<T>> vector_holder =
        std::make_shared<std::vector<T>>(std::move(data));
    // shared_ptr shouldn't own any memory, the memory is owned by vector_holder. As a result, we
    // pass a no-op deleter.
    data_ = std::shared_ptr<std::byte>(
        static_cast<std::byte*>(static_cast<void*>(vector_holder->data())), [](std::byte*) {});
    size_ = vector_holder->size() * sizeof(T);
    holder_ = vector_holder;
}

template <typename T, typename Deleter>
Memory::Memory(T* data, usize size, Deleter deleter) : size_{size} {
    data_.reset(static_cast<std::byte*>(static_cast<void*>(data)), deleter);
}
}
}  // namespace dw
