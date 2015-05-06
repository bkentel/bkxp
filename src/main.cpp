#include "bklib/assert.hpp"
#include "bklib/string.hpp"
#include "bklib/utility.hpp"
#include "bklib/math.hpp"
#include "bklib/exception.hpp"
#include "map.hpp"
#include "system.hpp"
#include "renderer.hpp"
#include "random.hpp"
#include "bsp_layout.hpp"


namespace bklib {

struct icolor {
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

} // namespace bklib

namespace bkrl {

using symbol_t = bklib::tagged_value<uint32_t, struct tag_symbol_t>;
using color_t  = bklib::icolor;

constexpr color_t color_black {0xFF, 0x00, 0x00, 0x00};

struct terrain_def_t {
    bklib::utf8_string name   {"nothing"};
    bklib::utf8_string flavor {"nothingness"};
    map_terrain_t type;
    symbol_t      symbol {' '};
    color_t       color = color_black;
};


struct room_generator {
    void generate(bkrl::random_t& gen, bklib::irect const& bounds) {

    }
};

} //namespace bkrl

template <typename T>
struct closed_interval {
    closed_interval(T const lo, T const hi, std::true_type) noexcept
      : lo {lo < hi ? lo : hi}
      , hi {lo < hi ? hi : lo}
    {
    }

    closed_interval(T const lo, T const hi, std::false_type) noexcept
      : lo {lo}, hi {hi}
    {
    }

    closed_interval(T const lo = T {0}, T const hi = {0}) noexcept
      : closed_interval(lo, hi, std::false_type {})
    {
    }

    closed_interval intersect(closed_interval const other) const noexcept {
        return {std::max(lo, other.lo), std::min(hi, other.hi)};
    }

    closed_interval normalize() const noexcept {
        return {lo, hi, std::true_type {}};
    }

    explicit operator bool() const noexcept {
        return lo <= hi;
    }

    T lo;
    T hi;
};

void main() try {
    bkrl::map map;

    bkrl::random_t gen {1984};

    bkrl::bsp_layout layout {bklib::irect {0, 0, 100, 100}};
    bkrl::room_generator room_gen;
    
    layout.generate(gen, [&](bklib::irect const& bounds) {
        room_gen.generate(gen, bounds);
    });

    bkrl::system sys;
    bkrl::renderer render(sys);

    sys.on_text_input = [&](bklib::utf8_string_view s) {
        printf("foo\n");
    };

    while (sys.is_running()) {
        sys.do_events_wait();
        render.clear();
        render.present();
    }

    return;
} catch (bklib::exception_base const&) {
} catch (boost::exception const&) {
} catch (std::exception const&) {
} catch (...) {
}
