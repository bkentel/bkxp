#pragma once

#include <vector>
#include <iterator>
#include <algorithm>

namespace bklib {

//--------------------------------------------------------------------------------------------------
//! @return a pointer to the first item matching the predicate @p p in the container @p c.
//! Otherwise, return nullptr.
//--------------------------------------------------------------------------------------------------
template <typename Container, typename Predicate>
inline decltype(auto) find_maybe(Container&& c, Predicate&& p) {
    static_assert(!std::is_rvalue_reference<Container&&>::value, "bad reference type");

    using std::begin;
    using std::end;

    auto const last = end(c);
    auto const it = std::find_if(begin(c), last, p);

    return (it != last) ? std::addressof(*it) : nullptr;
}

template <typename T, typename Key, typename KeyCompare, typename PosCompare>
class spatial_map {
public:
    void insert(T&& data) {
        data_.emplace_back(std::move(data));
    }

    T* operator[](Key const& id) {
        return find_maybe(data_, [&](T const& data) {
            return KeyCompare {}(id, data);
        });
    }

    T const* operator[](Key const& id) const {
        return (*const_cast<spatial_map*>(this))[id];
    }

    T* at(int const x, int const y) {
        return find_maybe(data_, [&](T const& data) {
            return PosCompare {}(data, x, y);
        });
    }

    T const* at(int const x, int const y) const {
        return const_cast<spatial_map*>(this)->at(x, y);
    }

    decltype(auto) begin() { return std::begin(data_); }
    decltype(auto) end()   { return std::end(data_); }

    decltype(auto) begin() const { return std::begin(data_); }
    decltype(auto) end()   const { return std::end(data_); }
private:
    std::vector<T> data_;
};

} //namespace bklib
