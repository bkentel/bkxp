#include "equip.hpp"
#include "item.hpp"

#include "bklib/assert.hpp"
#include "bklib/flag_set.hpp"
#include "bklib/algorithm.hpp"

namespace {

constexpr bkrl::equipment::result_t
make_result(
    bkrl::equipment::result_t::status_t const status_value
  , bkrl::equip_slot const slot
) noexcept {
    return {status_value, static_cast<std::underlying_type_t<bkrl::equip_slot>>(slot)};
}

constexpr bkrl::equipment::result_t
make_result(
    bkrl::equipment::result_t::status_t const status_value
  , bkrl::item_slots const flags
) noexcept {
    return {status_value, flags.value.flags};
}

} //namespace

//--------------------------------------------------------------------------------------------------
bkrl::equipment::equipment()
{
    slots_.insert(begin(slots_), {
        slot_t {equip_slot::hand_main, "right hand", nullptr}
      , slot_t {equip_slot::hand_off,  "left hand",  nullptr}
      , slot_t {equip_slot::head,      "head",       nullptr}
      , slot_t {equip_slot::torso,     "torso",      nullptr}
    });
}

//--------------------------------------------------------------------------------------------------
bool bkrl::equipment::is_equipped(item const& i) const noexcept
{
    auto islots = i.slots();
    for (auto const& slot : slots_) {
        if (islots.none()) {
            break;
        }

        if (slot.itm != &i) {
            continue;
        }

        islots.clear(slot.type);

        if ( (slot.type == equip_slot::hand_main || slot.type == equip_slot::hand_off)
          && (islots.test(equip_slot::hand_any))
        ) {
            islots.clear(equip_slot::hand_any);
        }
    }

    BK_PRECONDITION((islots.any() && islots == i.slots()) || islots.none());
    return islots.none();
}

//--------------------------------------------------------------------------------------------------
bool bkrl::equipment::is_equipped(equip_slot const es) const noexcept
{
    if (auto const slot = find_slot_(es)) {
        return !!slot->itm;
    }

    return false;
}

//--------------------------------------------------------------------------------------------------
bkrl::equipment::slot_t const*
bkrl::equipment::slot_info(equip_slot const es) const
{
    return find_slot_(es);
}

//--------------------------------------------------------------------------------------------------
bkrl::item_slots bkrl::equipment::where(item const& i) const noexcept
{
    using es = equip_slot;

    item_slots result {};

    for (auto const& slot : slots_) {
        if (slot.itm == &i) {
            BK_PRECONDITION((slot.type == es::hand_main || slot.type == equip_slot::hand_off)
              ? i.slots().any_of({es::hand_any, es::hand_main, es::hand_off})
              : i.slots().test(slot.type));

            result.set(slot.type);
        }
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
bkrl::equipment::result_t
bkrl::equipment::can_equip(item const & i) const
{
    if (!i.flags().test(item_flag::is_equippable)) {
        return {result_t::status_t::not_equippable};
    }

    if (i.flags().test(item_flag::is_equipped)) {
        return {result_t::status_t::already_equipped};
    }

    return {result_t::status_t::ok};
}

//--------------------------------------------------------------------------------------------------
bkrl::equipment::result_t
bkrl::equipment::equip(item& i)
{
    //
    // first check whether this is something that can actually be equipped.
    //
    {
        auto const result = can_equip(i);
        if (!result) {
            return result;
        }
    }

    auto const islots = i.slots();
    using es = equip_slot;

    //
    // next check whether the slot requirements can be met
    //
    {
        auto required_slots = islots;
        auto occupied_slots = item_slots {};

        for (auto const& slot : slots_) {
            if (slot.itm && required_slots.test(slot.type)) {
                occupied_slots.set(slot.type);
            }

            required_slots.clear(slot.type);

            if ( (required_slots.test(es::hand_any))
              && (slot.type == es::hand_main || slot.type == es::hand_off)
            ) {
                required_slots.clear(es::hand_any);
            }
        }

        if (occupied_slots.any()) {
            return make_result(result_t::status_t::slot_occupied, occupied_slots);
        }

        if (required_slots.any()) {
            return make_result(result_t::status_t::slot_not_present, required_slots);
        }
    }

    auto slots = islots;
    for (auto& slot : slots_) {
        if (slots.none()) {
            break;
        }

        if ( (slots.test(es::hand_any))
          && (slot.type == es::hand_main || slot.type == es::hand_off)
          && (slots.test(es::hand_any))
          && (!slot.itm)
        ) {
            slots.clear(es::hand_any);
        } else if (!slots.test(slot.type)) {
            continue;
        } else {
            slots.clear(slot.type);
        }

        BK_ASSERT(!slot.itm);
        slot.itm = &i;
    }

    if (slots.any()) {
        return make_result(result_t::status_t::slot_occupied, slots);
    }

    BK_ASSERT(!i.flags().test(item_flag::is_equipped));
    i.flags().set(item_flag::is_equipped);

    return make_result(result_t::status_t::ok, islots);
}

//--------------------------------------------------------------------------------------------------
bkrl::equipment::result_t
bkrl::equipment::unequip(equip_slot const es)
{
    auto const found_slot = find_slot_(es);
    if (!found_slot) {
        return make_result(result_t::status_t::slot_not_present, es);
    }

    auto const itm = found_slot->itm;
    if (!itm) {
        return make_result(result_t::status_t::slot_empty, es);
    }

    auto occupied_slots = item_slots {};
    for (auto& slot : slots_) {
        if (slot.itm != itm) {
            continue;
        }

        occupied_slots.set(slot.type);
        slot.itm = nullptr;
    }

    if (occupied_slots != itm->slots()) {
        BK_ASSERT(false); // TODO
    }

    BK_ASSERT(itm->flags().test(item_flag::is_equipped));
    itm->flags().clear(item_flag::is_equipped);

    return make_result(result_t::status_t::ok, occupied_slots);
}

bkrl::equipment::slot_t const*
bkrl::equipment::find_slot_(equip_slot const slot) const
{
    return bklib::find_maybe(slots_, [slot](auto const& s) noexcept {
        return s.type == slot;
    });
}
