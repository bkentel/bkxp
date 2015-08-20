#pragma once

#include <memory>

namespace bklib {

template <typename T> class simple_promise;

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
template <typename T>
class simple_future {
    friend class simple_promise<T>;
public:
    simple_future() = default;

    bool valid() const noexcept {
        return data_ && data_->ready;
    }

    T get() {
        if (!valid()) {
            throw "invalid";
        }

        return data_->value;
    }
private:
    struct data_t {
        T    value;
        bool ready;
    };

    explicit simple_future(std::shared_ptr<data_t>& ptr)
      : data_ {ptr}
    {
    }

    std::shared_ptr<data_t> data_;
};

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
template <typename T>
class simple_promise {
public:
    using future_t = simple_future<T>;
    using data_t = typename future_t::data_t;

    template <typename U>
    void set_value(U&& value) {
        if (data_) {
            data_->value = std::forward<U>(value);
            data_->ready = true;
        } else {
            data_ = std::make_shared<data_t>(data_t {std::forward<T>(value), true});
        }
    }

    future_t get_future() {
        if (!data_) {
            data_t data;
            std::fill_n(reinterpret_cast<char*>(&data.value), sizeof(data.value), 0);
            data.ready = false;

            data_ = std::make_shared<data_t>(std::move(data));
        }

        return future_t {data_};
    }
private:
    std::shared_ptr<data_t> data_;
};

} //namespace bkrl
