#pragma once

namespace bklib {
////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename Tag>
struct tagged_value {
    using value_type = T;
    using tag_type   = Tag;

    explicit tagged_value(T const value) : value {value} { }

    T value;
};

template <typename T, typename Tag>
bool operator==(tagged_value<T, Tag> const& lhs, tagged_value<T, Tag> const& rhs) noexcept {
    return lhs.value == rhs.value;
}

template <typename T, typename Tag>
bool operator!=(tagged_value<T, Tag> const& lhs, tagged_value<T, Tag> const& rhs) noexcept {
    return !(lhs.value == rhs.value);
}

template <typename T, typename Tag>
constexpr T value_cast(tagged_value<T, Tag> const& n) noexcept {
    return n.value;
}


} // namespace bklib
////////////////////////////////////////////////////////////////////////////////////////////////////
