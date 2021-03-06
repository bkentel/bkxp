#pragma once

#include "bklib/math.hpp"
#include "bklib/assert.hpp"
#include "bklib/algorithm.hpp"

#include <vector>
#include <iterator>
#include <algorithm>

namespace bklib {

//--------------------------------------------------------------------------------------------------
//! 2D spatial index
//--------------------------------------------------------------------------------------------------
template <typename T>
class spatial_map_2d {
public:
    using point_t = bklib::ipoint2;
    using rect_t  = bklib::irect;

    //----------------------------------------------------------------------------------------------
    //! @pre @p data isn't already in the map
    //----------------------------------------------------------------------------------------------
    T& insert(point_t p, T&& data) {
        auto const i = static_cast<int>(data_.size());
        data_.emplace_back(std::move(data));

        sorted_.emplace_back(std::make_pair(std::move(p), i));
        sort_();

        return data_.back();
    }

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    bool relocate(point_t const from, point_t const to, T const& data) {
        if (from == to) {
            return true;
        }

        auto const last = end(sorted_);
        auto const first = begin(sorted_);
        auto const it = std::find_if(first, last, [&](std::pair<point_t, int> const pair) {
            return (pair.first == from) && (get_data_at_(pair) == std::addressof(data));
        });

        if (it == last) {
            return false;
        }

        it->first = to;

        if (it != first && std::next(it, -1)->first < to) {
            auto const next = std::next(it, 1);
            if (next != last && to < next->first) {
                return true;
            }
        }

        sort_();

        return true;
    }

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    T remove(point_t const p) {
        auto const last = end(sorted_);
        auto const it = std::find_if(begin(sorted_), last, [&](auto const& pair) {
            return pair.first == p;
        });

        BK_PRECONDITION(it != last);

        auto const index = it->second;
        auto const data_it = std::next(data_.begin(), index);

        T result = std::move(*data_it);

        data_.erase(data_it);
        sorted_.erase(it);

        for (auto& pair : sorted_) {
            if (pair.second > index) {
                --pair.second;
            }
        }

        return result;
    }

    //----------------------------------------------------------------------------------------------
    //! TODO: this is kind of a hack related to the current implementation
    //----------------------------------------------------------------------------------------------
    template <typename Predicate, typename Remove>
    void remove_if(Predicate&& predicate, Remove&& do_remove) {
        for (auto i = 0u; i < sorted_.size(); ) {
            auto const& p = sorted_[i];
            auto& d = data_[static_cast<size_t>(p.second)];
            if (predicate(p.first, d)) {
                auto const size = data_.size();
                do_remove(p.first, d);
                BK_ASSERT(data_.size() < size);
            } else {
                ++i;
            }
        }
    }

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    void remove(point_t, T const&) {
    }

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    T* at(point_t const& p) {
        auto const i = find_maybe(sorted_, [&](auto const& pair) {
            return pair.first == p;
        });

        return i ? get_data_at_(*i) : nullptr;
    }

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    T const* at(point_t const& p) const {
        return const_cast<spatial_map_2d*>(this)->at(p);
    }

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    template <typename F>
    void for_each_data(F&& func) const {
        for (auto const& d : data_) {
            func(d);
        }
    }

    template <typename F>
    void for_each_data(F&& func) {
        for (auto& d : data_) {
            func(d);
        }
    }

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    template <typename F>
    void for_each_at(rect_t const& r, F&& func) const {
        using bklib::intersects;

        for (auto& pair : sorted_) {
            if (intersects(pair.first, r)) {
                func(pair.first, *get_data_at_(pair));
            }
        }
    }

    //----------------------------------------------------------------------------------------------
    //!
    //----------------------------------------------------------------------------------------------
    template <typename Predicate>
    T const* find(Predicate&& pred) const {
        return find_maybe(data_, std::forward<Predicate>(pred));
    }

    template <typename Predicate>
    T* find(Predicate&& pred) {
        return find_maybe(data_, std::forward<Predicate>(pred));
    }
private:
    T* get_data_at_(std::pair<point_t, int> const p) noexcept {
        BK_PRECONDITION(p.second >= 0);
        return std::addressof(data_[static_cast<size_t>(p.second)]);
    }

    T const* get_data_at_(std::pair<point_t, int> const p) const noexcept {
        return const_cast<spatial_map_2d*>(this)->get_data_at_(p);
    }

    void sort_() {
        std::sort(begin(sorted_), end(sorted_), [](auto const& lhs, auto const& rhs) {
            return lhs.first < rhs.first;
        });
    }

    std::vector<T>                       data_;
    std::vector<std::pair<point_t, int>> sorted_;
};

} //namespace bklib
