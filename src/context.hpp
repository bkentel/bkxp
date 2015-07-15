#pragma once

#include "random.hpp"
#include "definitions.hpp"
#include "output.hpp"

namespace bkrl {

struct context {
    context(context const&) = delete;
    context(context&&) = delete;
    context& operator=(context const&) = delete;
    context& operator=(context&&) = delete;

    random_state&      random;
    definitions const& data;
    output&            out;
};

} //namespace bkrl
