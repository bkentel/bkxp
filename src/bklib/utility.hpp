#pragma once

#include "string.hpp"

#include <vector>
#include <array>
#include <type_traits>
#include <cstring>
#include <cstdint>

namespace bklib {
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
//! Used for strict-aliasing-safe type punning.
//! @pre sizeof(from) and sizeof(to) must match.
//! @pre from and to must be trivially copyable.
//--------------------------------------------------------------------------------------------------
template <typename From, typename To>
inline decltype(auto) pseudo_cast(From const& from, To&& to) noexcept {
    using from_base_t = std::remove_reference_t<From>;
    using to_base_t   = std::remove_reference_t<To>;

    constexpr size_t const size_from = sizeof(from_base_t);
    constexpr size_t const size_to   = sizeof(to_base_t);

    static_assert(std::is_trivially_copyable<from_base_t>::value, "invalid from type");
    static_assert(std::is_trivially_copyable<to_base_t>::value, "invalid to type");
    static_assert(size_from == size_to, "size mismatch");

    std::memcpy(std::addressof(to), std::addressof(from), size_from);
    return std::forward<To>(to);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// tagged_value
////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename Tag, typename T>
class tagged_value {
public:
    using type = T;

    explicit tagged_value(T value) noexcept
      : value_ {std::move(value)}
    {
        static_assert(std::is_nothrow_constructible<T, T>::value, "");
    }

    tagged_value() noexcept
      : tagged_value {T {}}
    {
        static_assert(std::is_default_constructible<T>::value, "");
    }

    explicit operator bool() const noexcept {
        return !!value_;
    }

    explicit operator T() const noexcept {
        static_assert(std::is_nothrow_copy_constructible<T>::value, "");
        return value_;
    }
private:
    T value_;
};

template <typename Tag, typename T>
inline bool operator==(tagged_value<Tag, T> const& lhs, tagged_value<Tag, T> const& rhs) noexcept {
    return static_cast<T>(lhs) == static_cast<T>(rhs);
}

template <typename Tag, typename T>
inline bool operator!=(tagged_value<Tag, T> const& lhs, tagged_value<Tag, T> const& rhs) noexcept {
    return !(lhs == rhs);
}

template <typename Tag, typename T>
inline bool operator<(tagged_value<Tag, T> const& lhs, tagged_value<Tag, T> const& rhs) noexcept {
    return static_cast<T>(lhs) < static_cast<T>(rhs);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// hash_id
////////////////////////////////////////////////////////////////////////////////////////////////////

template <size_t N>
decltype(auto) to_array(utf8_string_view const src) noexcept {
    std::array<char, N> result;

    if (auto const n = std::min(N - 1, src.size())) {
        std::copy_n(std::begin(src), n, std::begin(result));
        result[n] = 0;
    } else {
        result[0] = 0;
    }

    return result;
}

template <size_t Extra = 12, typename T = std::uint32_t>
class hash_id_base {
public:
    static_assert(std::is_integral<T>::value, "");

    using type = T;
    static constexpr auto const extra = (((Extra + sizeof(T)) % sizeof(size_t) ? 1u : 0u)
                                      + (Extra + sizeof(T)) / sizeof(size_t)) * sizeof(size_t)
                                      - sizeof(T);

    static_assert(extra > 0, "");

    hash_id_base(hash_id_base const&) = default;
    hash_id_base& operator=(hash_id_base const&) = default;
    hash_id_base(hash_id_base&&) = default;
    hash_id_base& operator=(hash_id_base&&) = default;

    hash_id_base() noexcept
      : value_  {}
      , string_ {}
    {
    }

    explicit hash_id_base(T const value) noexcept
      : value_ {value}
    {
        string_[0] = 0;
    }

    hash_id_base(utf8_string_view const str) noexcept
      : value_  {djb2_hash(str)}
      , string_ {to_array<extra>(str)}
    {
    }

    hash_id_base(char const* const str, size_t const len) noexcept
      : hash_id_base {utf8_string_view {str, len}}
    {
    }

    template <size_t N>
    hash_id_base(hash_id_base<N, T> const& other) {
        *this = other;
    }

    template <size_t N>
    hash_id_base& operator=(hash_id_base<N, T> const& rhs) {
        value_ = rhs.value_;

        auto const n = std::min(string_.size(), rhs.string_.size());
        std::copy_if(
            begin(rhs.string_), end(rhs_.string_)
          , begin(string_)
          , [n, i = 0u](auto const& c) { return c && i++ < n; }
        );

        return *this;
    }

    void reset() noexcept {
        *this = hash_id_base {};
    }

    void reset(utf8_string_view const str) noexcept {
        *this = hash_id_base {str};
    }

    void reset(char const* const str, size_t const len) noexcept {
        *this = hash_id_base {str, len};
    }

    explicit operator T() const noexcept {
        return value_;
    }

    explicit operator bool() const noexcept {
        return !!value_;
    }

    char const* c_str() const noexcept {
        return string_.data();
    }
private:
    T value_;
    std::array<char, extra> string_;
};

template <typename T>
class hash_id_base<0, T> {
public:
    using type = T;
    static constexpr auto const extra = 0;

    hash_id_base() noexcept
      : value_ {}
    {
    }

    explicit hash_id_base(T const value) noexcept
      : value_ {value}
    {
    }

    hash_id_base(utf8_string_view const str) noexcept
      : value_ {djb2_hash(str)}
    {
    }

    explicit operator T() const noexcept {
        return value_;
    }

    explicit operator bool() const noexcept {
        return !!value_;
    }

    char const* c_str() const noexcept {
        return nullptr;
    }
private:
    T value_;
};

template <typename Tag, typename T = std::uint32_t, size_t Extra = 12>
class hash_id : public hash_id_base<Extra, T> {
public:
    using hash_id_base<Extra, T>::hash_id_base;
};

template <typename Tag, typename T, size_t E>
inline bool operator==(hash_id<Tag, T, E> const& lhs, utf8_string_view const rhs) noexcept {
    return static_cast<T>(lhs) == djb2_hash(rhs);
}

template <typename Tag, typename T, size_t E>
inline bool operator==(utf8_string_view const lhs, hash_id<Tag, T, E> const& rhs) noexcept {
    return (rhs == lhs);
}

template <typename Tag, typename T, size_t E>
inline bool operator!=(hash_id<Tag, T, E> const& lhs, utf8_string_view const rhs) noexcept {
    return !(lhs == rhs);
}

template <typename Tag, typename T, size_t E>
inline bool operator!=(utf8_string_view const lhs, hash_id<Tag, T, E> const& rhs) noexcept {
    return !(lhs == rhs);
}

template <typename Tag, typename T, size_t E0, size_t E1>
inline bool operator==(hash_id<Tag, T, E0> const& lhs, hash_id<Tag, T, E1> const& rhs) noexcept {
    return static_cast<T>(lhs) == static_cast<T>(rhs);
}

template <typename Tag, typename T, size_t E0, size_t E1>
inline bool operator!=(hash_id<Tag, T, E0> const& lhs, hash_id<Tag, T, E1> const& rhs) noexcept {
    return !(lhs == rhs);
}

template <typename Tag, typename T, size_t E0, size_t E1>
inline bool operator<(hash_id<Tag, T, E0> const& lhs, hash_id<Tag, T, E1> const& rhs) noexcept {
    return static_cast<T>(lhs) < static_cast<T>(rhs);
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
std::vector<char> read_file_to_buffer(utf8_string_view filename);

} // namespace bklib
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace std {
template <typename T, typename Tag> struct hash<bklib::tagged_value<T, Tag>> {
    inline size_t operator()(bklib::tagged_value<T, Tag> const& k) const noexcept {
        return k.value;
    }
};
} //namespace std
