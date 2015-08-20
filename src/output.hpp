#pragma once

#include "bklib/string.hpp"
#include "external/format.h"

#include <functional>
#include <vector>

namespace bkrl {

using output_sink = std::function<void(bklib::utf8_string_view)>;

class output {
public:
    bool write(fmt::MemoryWriter const& mem) {
        do_write(bklib::utf8_string_view {mem.data(), mem.size()});
        return true;
    }

    template <size_t N>
    bool write(char const (&str)[N]) {
        do_write(bklib::make_string_view(str));
        return true;
    }

    bool write(bklib::utf8_string_view const str) {
        do_write(str);
        return true;
    }

    template <typename Arg0, typename... Args>
    bool write(char const* const format,  Arg0 const& arg0, Args const&... args) {
        out_.write(format, arg0, args...);
        do_write();
        return true;
    }

    template <typename Arg0, typename... Args>
    bool write(bklib::utf8_string const& format, Arg0 const& arg0, Args const&... args) {
        out_.write(format, arg0, args...);
        do_write();
        return true;
    }

    template <typename... Args>
    bool write_if(bool const cond, char const* const format, Args const&... args) {
        if (cond) {
            write(format, args...);
        }

        return cond;
    }

    template <typename... Args>
    bool write_if(bool const cond, bklib::utf8_string const& format, Args const&... args) {
        if (cond) {
            write(format, args...);
        }

        return cond;
    }

    size_t push(output_sink sink) {
        sinks_.push_back(sink);
        return sinks_.size();
    }
private:
    void do_write() {
        do_write(bklib::utf8_string_view {out_.data(), out_.size()});
    }

    void do_write(bklib::utf8_string_view const str) {
        for (auto& sink : sinks_) {
            sink(str);
        }

        out_.clear();
    }

    fmt::MemoryWriter        out_;
    std::vector<output_sink> sinks_;
};

} //namespace bkrl
