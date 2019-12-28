/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include "../Base.h"
#include <functional>

namespace dw {
namespace gfx {
// Type safe handles.
template <typename HandleType> class BaseHandle {
public:
    using base_type = u32;

    BaseHandle() : value_(-1) {}

    explicit BaseHandle(base_type value) : value_(value) {
    }

    explicit operator base_type() const {
        return value_;
    }

    HandleType& operator=(base_type other) {
        value_ = other;
        return static_cast<HandleType&>(*this);
    }

    bool operator==(const BaseHandle& other) const {
        return value_ == other.value_;
    }

    bool operator==(base_type other) const {
        return value_ == other;
    }

    bool operator!=(const BaseHandle& other) const {
        return value_ != other.value_;
    }

    bool operator!=(base_type other) const {
        return value_ != other;
    }

    HandleType& operator++() {
        ++value_;
        return static_cast<HandleType&>(*this);
    }

    HandleType operator++(int) {
        BaseHandle tmp{*this};
        ++value_;
        return static_cast<HandleType&>(tmp);
    }

private:
    base_type value_;
};

// Handle generator.
template <typename Handle> class HandleGenerator {
public:
    HandleGenerator() : next_{1} {
    }

    void reset() {
        next_ = Handle(1);
    }

    Handle next() {
        return next_++;
    }

private:
    Handle next_;
};
}  // namespace gfx
}  // namespace dw

namespace std {
template <typename HandleType> struct hash<dw::gfx::BaseHandle<HandleType>> {
    using argument_type = dw::gfx::BaseHandle<HandleType>;
    using result_type = std::size_t;
    using base_type = typename dw::gfx::BaseHandle<HandleType>::base_type;
    result_type operator()(const argument_type& k) const {
        std::hash<base_type> base_hash;
        return base_hash(base_type(k));
    }
};
}  // namespace std
