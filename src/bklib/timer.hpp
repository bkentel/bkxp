#pragma once

#include "bklib/utility.hpp"
#include <chrono>
#include <functional>
#include <deque>
#include <utility>

namespace bklib {

//--------------------------------------------------------------------------------------------------
//! 
//--------------------------------------------------------------------------------------------------
class timer {
public:
    struct record_t;

    using id_t       = bklib::tagged_value<int, timer>;
    using callback_t = std::function<void (record_t& r)>;
    using duration_t = std::chrono::microseconds;

    struct record_t {
        record_t() = delete;

        id_t       id;
        duration_t duration;
        callback_t callback;
        bool       repeat;
    };

    bool empty() const noexcept;
    int size() const noexcept;

    int update();
    bool remove(id_t id);

    bool reset(id_t id);

    template <typename Duration>
    bool reset(id_t const id, Duration const duration) {
        return reset_(id, std::chrono::duration_cast<duration_t>(duration));
    }

    template <typename Duration, typename Callback>
    id_t add(Duration const duration, Callback&& callback, bool const repeat = true) {
        return add_(record_t {
            id_t {++next_id_}
          , std::chrono::duration_cast<duration_t>(duration)
          , std::forward<Callback>(callback)
          , repeat
        });
    }
private:
    using time_point_t = std::chrono::high_resolution_clock::time_point;
    using pair_t       = std::pair<time_point_t, record_t>;

    id_t add_(record_t&& rec);
    id_t add_(time_point_t now, record_t&& rec);

    bool reset_(id_t id, duration_t duration, bool use_new_duration = true);

    int                next_id_ = 0;
    std::deque<pair_t> records_;
    bool updating_ = false;
};

} //namespace bklib
