#pragma once

#include <utility>
#include <type_traits>
#include <initializer_list>

namespace bklib {

namespace detail {

template <typename T>
struct flag_set_base {
    static_assert(std::is_unsigned<T>::value, "");

    flag_set_base() = default;
    flag_set_base(T const value) noexcept : flags {value_of(value)} {}

    static constexpr T value_of(T const flag) noexcept {
        return T {1} << flag;
    }

    void set(T const flag) noexcept {
        flags |= value_of(flag);
    }

    void set(std::initializer_list<T> const flag_list) noexcept {
        for (auto const flag : flag_list) {
            set(flag);
        }
    }

    void clear() noexcept {
        flags = T {0};
    }

    void clear(T const flag) noexcept {
        flags &= ~value_of(flag);
    }

    void clear(std::initializer_list<T> const flag_list) noexcept {
        for (auto const flag : flag_list) {
            clear(flag);
        }
    }

    bool test(T const flag) const noexcept {
        return !!(flags & value_of(flag));
    }

    bool none() const noexcept {
        return !flags;
    }

    bool none_of(std::initializer_list<T> const flag_list) const noexcept {
        return !any_of(flag_list);
    }

    bool any() const noexcept {
        return !!flags;
    }

    bool any_of(std::initializer_list<T> const flag_list) const noexcept {
        bool result = false;

        for (auto const flag : flag_list) {
            if ((result = test(flag))) {
                break;
            }
        }

        return result;
    }

    T flags {};
};

} //namespace detail

//--------------------------------------------------------------------------------------------------
//! The actual implementation is forwarded to detail::flag_set_base to hopefully reduce the
//! amount of code generated from one instance per enum to one instance to per underlying type.
//--------------------------------------------------------------------------------------------------
template <typename Enum>
struct flag_set {
    static_assert(std::is_enum<Enum>::value, "");

    using type = std::underlying_type_t<Enum>;

    static constexpr auto bits = sizeof(type) * 8;

    flag_set() = default;
    flag_set(Enum const flag) noexcept : value {value_of(flag)} {}

    static constexpr type value_of(Enum const flag) noexcept {
        return static_cast<type>(flag);
    }

    static constexpr std::initializer_list<type>
    value_of(std::initializer_list<Enum> const flag_list) noexcept {
        return reinterpret_cast<std::initializer_list<type> const&>(flag_list);
    }

    void set(Enum const flag) noexcept {
        value.set(value_of(flag));
    }

    void set(std::initializer_list<Enum> const flag_list) noexcept {
        return value.set(value_of(flag_list));
    }

    void clear() noexcept {
        value.clear();
    }

    void clear(Enum const flag) noexcept {
        value.clear(value_of(flag));
    }

    void clear(std::initializer_list<Enum> const flag_list) noexcept {
        value.clear(value_of(flag_list));
    }

    bool test(Enum const flag) const noexcept {
        return value.test(value_of(flag));
    }

    bool none() const noexcept {
        return value.none();
    }

    bool none_of(std::initializer_list<Enum> const flag_list) const noexcept {
        return value.none_of(value_of(flag_list));
    }

    bool any() const noexcept {
        return value.any();
    }

    bool any_of(std::initializer_list<Enum> const flag_list) const noexcept {
        return value.any_of(value_of(flag_list));
    }

    detail::flag_set_base<type> value;
};

template <typename T>
constexpr inline bool operator==(flag_set<T> const lhs, flag_set<T> const rhs) noexcept {
    return lhs.value.flags == rhs.value.flags;
}

template <typename T>
constexpr inline bool operator!=(flag_set<T> const lhs, flag_set<T> const rhs) noexcept {
    return !(lhs == rhs);
}

namespace detail {

template <typename T, typename Function>
void for_each_flag(flag_set<T> const flags, Function&& function, std::true_type) {
    constexpr auto const bits = sizeof(flags.value) * 8;
    for (auto i = 0u; i < bits; ++i) {
        auto const bit = static_cast<T>(i);
        if (!flags.test(bit)) {
            continue;
        }

        if (!function(bit)) {
            return;
        }
    }
}

template <typename T, typename Function>
void for_each_flag(flag_set<T> const flags, Function&& function, std::false_type) {
    constexpr auto const bits = sizeof(flags.value) * 8;
    for (auto i = 0u; i < bits; ++i) {
        auto const bit = static_cast<T>(i);
        if (flags.test(bit)) {
            function(bit);
        }
    }
}

} //namespace detail

template <typename T, typename Function>
void for_each_flag(flag_set<T> const flags, Function&& function) {
    detail::for_each_flag(
        flags
      , std::forward<Function>(function)
      , std::is_same<decltype(function(std::declval<T>())), bool> {}
    );
}

} //namespace bklib
