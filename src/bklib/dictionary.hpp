#pragma once

#include <vector>
#include <iterator>
#include <algorithm>

namespace bklib {

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
template <typename Definition>
class dictionary {
public:
    using id_type = typename Definition::id_type;

    //----------------------------------------------------------------------------------------------
    dictionary() = default;

    dictionary(dictionary const&) = delete;
    dictionary& operator=(dictionary const&) = delete;

    dictionary(dictionary&&) = default;
    dictionary& operator=(dictionary&&) = default;

    //----------------------------------------------------------------------------------------------
    bool           empty()  const noexcept { return data_.empty(); }
    auto           size()   const noexcept { return data_.size(); }
    decltype(auto) begin()  const noexcept { return data_.begin(); }
    decltype(auto) cbegin() const noexcept { return data_.cbegin(); }
    decltype(auto) end()    const noexcept { return data_.end(); }
    decltype(auto) cend()   const noexcept { return data_.cend(); }

    //----------------------------------------------------------------------------------------------
    Definition const* find(id_type const id) const {
        auto const it = lower_bound(data_, id);
        if (it == std::end(data_)) {
            return nullptr;
        }

        auto& value = *it;
        return (get_id(value) == id) ? std::addressof(value) : nullptr;
    }

    //----------------------------------------------------------------------------------------------
    template <typename T>
    decltype(auto) insert_or_discard(T&& value) {
        auto const it = lower_bound(data_, get_id(value));
        if (it != std::end(data_) && get_id(*it) == get_id(value)) {
            return std::make_pair(*it, false);
        }

        return std::make_pair(*data_.insert(it, std::forward<T>(value)), true);
    }

    template <typename T>
    decltype(auto) insert_or_replace(T&& value) {
        auto const it = lower_bound(data_, get_id(value));
        if (it != std::end(data_) && get_id(*it) == get_id(value)) {
            *it = std::forward<T>(value);
            return std::make_pair(*it, false);
        }

        return std::make_pair(*data_.insert(it, std::forward<T>(value)), true);
    }
private:
    //----------------------------------------------------------------------------------------------
    template <typename Container>
    static decltype(auto) lower_bound(Container&& c, id_type const id) noexcept {
        using std::begin;
        using std::end;

        return std::lower_bound(begin(c), end(c), id, [](auto&& lhs, auto&& rhs) noexcept {
            return get_id(lhs) < get_id(rhs);
        });
    }

    std::vector<Definition> data_;
};

} //namespace bklib
