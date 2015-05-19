#include "terrain.hpp"

#include <unordered_map>

//--------------------------------------------------------------------------------------------------
class bkrl::detail::terrain_dictionary_impl {
public:
    explicit terrain_dictionary_impl(bklib::utf8_string_view const filename)
    {
        values_[terrain_entry {terrain_type::empty, 0}] =
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
