#include "timer.hpp"

#include "bklib/algorithm.hpp"
#include <iterator>

namespace {

inline decltype(auto) find_timer_id(bklib::timer::id_t const id) noexcept {
    return [id](auto const& p) { return id == p.second.id; };
}

template <typename TimePoint>
inline decltype(auto) find_timer_pos(TimePoint const when) noexcept {
    return [when](auto const& p) { return p.first > when; };
}

} // namespace

//--------------------------------------------------------------------------------------------------
bool bklib::timer::remove(id_t const id)
{
    auto const it = find_if(records_, find_timer_id(id));
    if (it != end(records_)) {
        records_.erase(it);
        return true;
    }

    return false;
}

bool bklib::timer::reset(id_t const id)
{
    return reset_(id, duration_t {}, false);
}

//--------------------------------------------------------------------------------------------------
void bklib::timer::update()
{
    auto const now = std::chrono::high_resolution_clock::now();

    do {
        auto const it = begin(records_);
        if (it == end(records_)) {
            break;
        }

        auto const& deadline = it->first;
        auto& data = it->second;

        if (deadline > now) {
            break;
        }

        data.callback(data);

        if (!data.repeat) {
            records_.erase(it);
        } else {
            auto old = std::move(data);
            records_.erase(it);
            add_(now, std::move(old));
        }
    } while (!records_.empty());
}

//--------------------------------------------------------------------------------------------------
bklib::timer::id_t bklib::timer::add_(record_t&& rec)
{
    return add_(std::chrono::high_resolution_clock::now(), std::move(rec));
}

//--------------------------------------------------------------------------------------------------
bklib::timer::id_t
bklib::timer::add_(time_point_t const now, record_t&& rec)
{
    id_t const result = rec.id;
    auto const when = now + rec.duration;

    records_.insert(
        find_if(records_, find_timer_pos(when))
      , std::make_pair(when, std::move(rec))
    );

    return result;
}

//--------------------------------------------------------------------------------------------------
bool bklib::timer::reset_(id_t const id, duration_t const duration, bool const use_new_duration)
{
    auto const where = find_if(records_, find_timer_id(id));
    if (where == end(records_)) {
        return false;
    }

    auto tmp = std::move(*where);
    records_.erase(where);

    auto& deadline = tmp.first;
    auto& record = tmp.second;

    if (use_new_duration) {
        record.duration = duration;
    }

    auto const new_deadline = record.duration + std::chrono::high_resolution_clock::now();
    deadline = new_deadline;

    records_.insert(
        find_if(records_, find_timer_pos(new_deadline))
      , std::move(tmp)
    );

    return true;
}
