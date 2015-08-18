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
    void write(char const* const format, Args const&... args) {
        out_.write(format, args...);
        do_write();
    }

    template <typename... Args>
    void write(bklib::utf8_string const& format, Args const&... args) {
        out_.write(format, args...);
        do_write();
    }

    size_t push(output_sink sink) {
        sinks_.push_back(sink);
        return sinks_.size();
    }
private:
    void do_write() {
        bklib::utf8_string_view const str {out_.data(), out_.size()};
        for (auto& sink : sinks_) {
            sink(str);
        }

        out_.clear();
    }

    fmt::MemoryWriter        out_;
    std::vector<output_sink> sinks_;
};

} //namespace bkrl
