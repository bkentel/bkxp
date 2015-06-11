#include "timer.hpp"

#include "bklib/algorithm.hpp"
#include <iterator>

//--------------------------------------------------------------------------------------------------
bool bklib::timer::remove(id_t const id)
{
    auto const it = find_if(records_, [id](pair_t const& p) {
        return id == p.second.id;
    });

    if (it != end(records_)) {
        records_.erase(it);
        return true;
    }

    return false;
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
    auto const result = rec.id;
    auto const when = now + rec.duration;

    auto const where = find_if(records_, [&](pair_t const& p) {
        return p.first > when;
    });

    records_.insert(where, std::make_pair(when, std::move(rec)));

    return result;
}
