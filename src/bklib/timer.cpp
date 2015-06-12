#include "timer.hpp"

#include "bklib/algorithm.hpp"
#include "bklib/assert.hpp"
#include "bklib/scope_guard.hpp"
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
    BK_PRECONDITION(!updating_);

    auto const it = find_if(records_, find_timer_id(id));
    if (it == end(records_)) {
        return false;
    }

    records_.erase(it);
    return true;
}

bool bklib::timer::reset(id_t const id)
{
    return reset_(id, duration_t {}, false);
}

//--------------------------------------------------------------------------------------------------
bool bklib::timer::empty() const noexcept
{
    return records_.empty();
}

//--------------------------------------------------------------------------------------------------
int bklib::timer::size() const noexcept
{
    return static_cast<int>(records_.size());
}

//--------------------------------------------------------------------------------------------------
int bklib::timer::update()
{
    BK_PRECONDITION(!updating_);
    
    int result = 0;
    updating_ = true;
    BK_SCOPE_EXIT {updating_ = false; };

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

        ++result;
        data.callback(data);

        if (!data.repeat) {
            records_.erase(it);
        } else {
            auto old = std::move(data);
            records_.erase(it);
            
            updating_ = false;
            add_(now, std::move(old));
            updating_ = true;
        }
    } while (!records_.empty());

    return result;
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
    BK_PRECONDITION(!updating_);

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
    BK_PRECONDITION(!updating_);

    auto const where = find_if(records_, find_timer_id(id));
    if (where == end(records_)) {
        return false;
    }

    auto tmp = std::move(*where);
    records_.erase(where);

    auto& record = tmp.second;

    if (use_new_duration) {
        record.duration = duration;
    }

    add_(std::move(record));

    return true;
}
