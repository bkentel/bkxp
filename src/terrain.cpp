#include "terrain.hpp"

#include "bklib/utility.hpp"
#include <unordered_map>
#include <functional>

namespace std {
template <> struct hash<::bkrl::terrain_entry> {
    size_t operator()(const ::bkrl::terrain_entry& k) const noexcept {
        return ::bklib::hash_value(k.type, k.variant);
    }
};
} //namespace std

//--------------------------------------------------------------------------------------------------
class bkrl::detail::terrain_dictionary_impl {
public:
    explicit terrain_dictionary_impl(bklib::utf8_string_view const filename)
    {
        values_[terrain_entry {0, terrain_flags::none, terrain_type::empty, 0}] =
            terrain_def {"empty", "empty space", " "};
    }

    terrain_def const* find(terrain_entry const entry) const
    {
        auto const result = values_.find(entry);
        if (result != std::end(values_)) {
            return &result->second;
        }

        return nullptr;
    }
private:
    std::unordered_map<terrain_entry, terrain_def> values_;
};

//--------------------------------------------------------------------------------------------------
bkrl::terrain_dictionary::~terrain_dictionary() = default;

//--------------------------------------------------------------------------------------------------
bkrl::terrain_dictionary::terrain_dictionary(bklib::utf8_string_view const filename)
  : impl_ {std::make_unique<detail::terrain_dictionary_impl>(filename)}
{
}

//--------------------------------------------------------------------------------------------------
bkrl::terrain_def const* bkrl::terrain_dictionary::find(terrain_entry const entry) const
{
    return impl_->find(entry);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::door
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
bkrl::door::door() noexcept
{
    uint64_t const value = 0;
    bklib::pseudo_cast(value, data);
}

//--------------------------------------------------------------------------------------------------
bkrl::door::door(terrain_entry const& entry) noexcept
{
    bklib::pseudo_cast(entry.data, data);
}

//--------------------------------------------------------------------------------------------------
bool bkrl::door::set_open_close(state const s) noexcept
{
    return (s == state::open)
      ? open()
      : close();
}

//--------------------------------------------------------------------------------------------------
bool bkrl::door::open() noexcept
{
    if (!is_open()) {
        data.flags = 1;
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::door::close() noexcept
{
    if (!is_closed()) {
        data.flags = 0;
        return true;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::door::is_open() const noexcept
{
    return data.flags == 1;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::door::is_closed() const noexcept
{
    return !is_open();
}

//--------------------------------------------------------------------------------------------------
uint64_t bkrl::door::to_data() const noexcept
{
    uint64_t result;
    return bklib::pseudo_cast(data, result);
}
