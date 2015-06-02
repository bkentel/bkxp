#pragma once

#include "bklib/math.hpp"

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

template <typename T>
class spatial_map_2d {
public:
    using point_t = bklib::ipoint2;
    using rect_t  = bklib::irect;

    //--------------------------------------------------------------------------------------------------
    //! @pre @p data isn't already in the map
    //--------------------------------------------------------------------------------------------------
    T& insert(point_t p, T&& data) {
        auto const i = static_cast<int>(data_.size());
        data_.emplace_back(std::move(data));
        
        sorted_.emplace_back(std::make_pair(std::move(p), i));
        sort_();

        return data_.back();
    }

    bool relocate(point_t const from, point_t to, T const& data) {
        auto const last = end(sorted_);
        auto const it = std::find_if(begin(sorted_), last, [&](auto const& pair) {
            return (std::addressof(pair.second) == std::addressof(data))
                && (pair.first == from);
        });

        bool const found = (it != last);
        if (found) {
            it->first = std::move(to);
            sort_();
        }

        return found;
    }

    void remove(point_t p) {
    }

    void remove(point_t p, T const& data) {
    }

    //int count(point_t const& p) const {
    //    auto const n = std::count_if(begin(sorted_), end(sorted_), [&](auto const& pair) {
    //        return pair.first == p;
    //    });

    //    return static_cast<int>(n);
    //}

    T* at(point_t const& p) {
        auto const i = find_maybe(sorted_, [&](auto const& pair) {
            return pair.first == p;
        });

        return i ? std::addressof(data_[i->second]) : nullptr;
    }

    T const* at(point_t const& p) const {
        return const_cast<spatial_map_2d*>(this)->at(p);
    }

    template <typename F>
    void for_each_at(rect_t const& r, F&& func) const {
        using bklib::intersects;

        for (auto& pair : sorted_) {
            if (intersects(pair.first, r)) {
                func(pair.first, data_[pair.second]);
            }
        }
    }
private:
    void sort_() {
        std::sort(begin(sorted_), end(sorted_), [](auto const& lhs, auto const& rhs) {
            return lhs.first < rhs.first;
        });
    }

    std::vector<T>                       data_;
    std::vector<std::pair<point_t, int>> sorted_;
};

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
