#include "bklib/assert.hpp"

#include <cstddef>

namespace bklib {

template <size_t N>
class arena {
public:
    arena() noexcept = default;

    ~arena() {
        ptr_ = nullptr;
    }

    arena(arena const&) = delete;
    arena& operator=(arena const&) = delete;

    char* allocate(size_t n);
    void deallocate(char* p, size_t n) noexcept;

    static constexpr size_t size() noexcept {
        return N;
    }

    size_t used() const noexcept {
        return static_cast<size_t>(ptr_ - buf_);
    }

    void reset() noexcept {
        ptr_ = buf_;
    }
private:
    static constexpr size_t alignment = 16;
    alignas(alignment) char buf_[N];
    char* ptr_ = buf_;

    bool pointer_in_buffer(char const* const p) const noexcept {
        return buf_ <= p && p <= buf_ + N;
    }
};

template <size_t N>
char* arena<N>::allocate(size_t const n)
{
    BK_ASSERT(pointer_in_buffer(ptr_) && "short_alloc has outlived arena");

    if (buf_ + N - ptr_ >= static_cast<ptrdiff_t>(n)) {
        char* r = ptr_;
        ptr_ += n;
        return r;
    }

    return static_cast<char*>(::operator new(n));
}

template <size_t N>
void arena<N>::deallocate(char* p, size_t const n) noexcept
{
    BK_ASSERT(pointer_in_buffer(ptr_) && "short_alloc has outlived arena");

    if (pointer_in_buffer(p)) {
        if (p + n == ptr_) {
            ptr_ = p;
        }
    } else {
        ::operator delete(p);
    }
}

template <typename T, size_t N>
class short_alloc
{
    template <typename U, size_t M> friend class short_alloc;
public:
    using value_type = T;
    using arena_t = arena<N>;

    template <typename U> struct rebind {
        using other = short_alloc<U, N>;
    };

    short_alloc(arena<N>& a) noexcept
      : a_ {a}
    {
    }

    template <typename U>
    short_alloc(short_alloc<U, N> const& a) noexcept
      : a_ {a.a_}
    {
    }

    short_alloc(short_alloc const&) = default;
    short_alloc& operator=(short_alloc const&) = delete;

    T* allocate(size_t const n) {
        return reinterpret_cast<T*>(a_.allocate(n*sizeof(T)));
    }

    void deallocate(T* const p, size_t const n) noexcept {
        a_.deallocate(reinterpret_cast<char*>(p), n*sizeof(T));
    }

    template <typename T1, size_t N1, typename T2, size_t N2>
    friend bool operator==(short_alloc<T1, N1> const& lhs, short_alloc<T2, N2> const& rhs) noexcept;
private:
    arena<N>& a_;
};

template <typename T, size_t N, typename U, size_t M>
inline bool operator==(short_alloc<T, N> const& lhs, short_alloc<U, M> const& rhs) noexcept {
    return (N == M) && (&lhs.a_ == &rhs.a_);
}

template <typename T, size_t N, typename U, size_t M>
inline bool operator!=(short_alloc<T, N> const& lhs, short_alloc<U, M> const& rhs) noexcept {
    return !(lhs == rhs);
}

} //namespace bklib
