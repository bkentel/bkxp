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

    template <typename T, std::enable_if_t<!std::is_same<T, id_type>::value>* = nullptr>
    Definition const* find(T const& key) const {
        return find(get_id(key));
    }

    //----------------------------------------------------------------------------------------------
    //! Inserts @p value into the dictionary iff it does not already exist, otherwise @p value is
    //! ignored.
    //----------------------------------------------------------------------------------------------
    template <typename T>
    decltype(auto) insert_or_discard(T&& value) {
        return do_insert_(std::forward<T>(value), [](auto&&) noexcept {});
    }

    //----------------------------------------------------------------------------------------------
    //! Inserts @p value into the dictionary if it does not already exist, otherwise the existing
    //! value is replaced by @p value.
    //----------------------------------------------------------------------------------------------
    template <typename T>
    decltype(auto) insert_or_replace(T&& value) {
        return do_insert_(std::forward<T>(value), [&](auto&& existing) noexcept {
            existing = std::forward<T>(value);
        });
    }
private:
    //----------------------------------------------------------------------------------------------
    template <typename T, typename Function>
    decltype(auto) do_insert_(T&& value, Function&& f) {
        using result_t = std::pair<Definition const*, bool>;

        auto const it = lower_bound(data_, get_id(value));
        if (it != std::end(data_) && get_id(*it) == get_id(value)) {
            f(*it);
            return result_t {std::addressof(*it), false};
        }

        return result_t {std::addressof(*data_.insert(it, std::forward<T>(value))), true};
    }

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
