#pragma once

#include "bklib/string.hpp"
#include "external/format.h"

#include <functional>
#include <vector>

namespace bkrl {

using output_sink = std::function<void(bklib::utf8_string_view)>;

class output {
public:
    template <typename... Args>
    void write(bklib::utf8_string_view const format, Args const&... args) {
        do_write(fmt::sprintf(format.data(), args...));
    }

    size_t push(output_sink sink) {
        sinks_.push_back(sink);
        return sinks_.size();
    }
private:
    void do_write(bklib::utf8_string&& str) {
        bklib::utf8_string_view const view {str};

        for (auto& sink : sinks_) {
            sink(view);
        }
    }

    std::vector<output_sink> sinks_;
};

} //namespace bkrl
