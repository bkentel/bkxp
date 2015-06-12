#pragma once

namespace bklib {

template <typename Function>
class scope_guard {
public:
    scope_guard() = delete;
    scope_guard(scope_guard const&) = delete;
    scope_guard& operator=(scope_guard const&) = delete;

    scope_guard(Function&& f) noexcept : f_ {std::move(f)} { }

    scope_guard(scope_guard&& other) noexcept
      : f_ {std::move(other.f_)}
      , active_ {other.active_}
    {
        rhs.dismiss();
    }

    scope_guard& operator=(scope_guard&& rhs) noexcept {
        f_ = std::move(rhs.f_);
        active_ = rhs.active_;

        rhs.dismiss();
    }

    ~scope_guard() {
        if (active_) {
            f_();
        }
    }

    void dismiss() noexcept {
        active_ = false;
    }
private:
    Function f_;
    bool active_ = true;
};

template <typename Function>
inline decltype(auto) make_scope_guard(Function&& f) noexcept {
    return scope_guard<Function>(std::forward<Function>(f));
}

namespace detail {

struct scope_guard_on_exit {};

template <typename Function>
inline decltype(auto) operator+(scope_guard_on_exit, Function&& f) noexcept {
    return scope_guard<Function>(std::forward<Function>(f));
}

} //namespace detail

#define BK_CONCATENATE_IMPL(s1, s2) s1 ## s2
#define BK_CONCATENATE(s1, s2) BK_CONCATENATE_IMPL(s1, s2)

#ifdef __COUNTER__
#   define BK_ANONYMOUS_VARIABLE(str) BK_CONCATENATE(str, __COUNTER__)
#else
#   define BK_ANONYMOUS_VARIABLE(str) BK_CONCATENATE(str, __LINE__)
#endif

#define BK_SCOPE_EXIT \
	auto BK_ANONYMOUS_VARIABLE(BK_SCOPE_EXIT_STATE) \
	= ::bklib::detail::scope_guard_on_exit() + [&]() -> void

#define BK_NAMED_SCOPE_EXIT(name) \
	auto name = ::bklib::detail::scope_guard_on_exit() + [&]() -> void

} //namespace bklib
