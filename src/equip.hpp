#pragma once

#include "item.hpp"

#include "bklib/string.hpp"

#include <vector>
#include <cstdint>

namespace bkrl {

enum class equip_result_t : int {
    ok
  , not_equippable
  , slot_occupied
  , slot_not_present
  , slot_empty
  , already_equipped
  , not_held //!< The item isn't held by the subject trying to equip the item
};

//--------------------------------------------------------------------------------------------------
//! An equipment list. It doesn't actually own the equipped items, but rather just observes them.
//! Move only.
//--------------------------------------------------------------------------------------------------
class equipment {
public:
    //----------------------------------------------------------------------------------------------
    //
    //----------------------------------------------------------------------------------------------
    struct slot_t {
        equip_slot              type;
        bklib::utf8_string_view name;
        item*                   itm;
    };

    //----------------------------------------------------------------------------------------------
    //
    //----------------------------------------------------------------------------------------------
    struct result_t {
        using status_t = equip_result_t;

        constexpr result_t() noexcept = default;
        constexpr result_t(status_t const status_value, uint32_t const data_value = 0) noexcept
          : status {status_value}, data {data_value}
        {
        }

        explicit operator bool() const noexcept {
            return status == status_t::ok;
        }

        item_slots slots() const noexcept {
            return *reinterpret_cast<item_slots const*>(&data);
        }

        equip_slot slot() const noexcept {
            return static_cast<equip_slot>(data);
        }

        status_t status = status_t::ok;
        uint32_t data   = 0;
    };

    equipment();

    bool is_equipped(item const& i) const noexcept;
    bool is_equipped(equip_slot es) const noexcept;

    item_slots where(item const& i) const noexcept;

    slot_t const* slot_info(equip_slot const es) const;

    result_t can_equip(item const& i) const;

    template <typename Callback>
    void eligible_slots(item const& i, Callback&& callback) const;

    result_t equip(item& i);

    result_t unequip(equip_slot const es);

    template <typename Predicate>
    void unequip(Predicate&& predicate);
private:
    slot_t const* find_slot_(equip_slot const slot) const;

    std::vector<slot_t> slots_;
};

} //namespace bkrl
