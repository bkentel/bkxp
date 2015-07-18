#pragma once

namespace bkrl {

class item_factory;
class creature_factory;
class output;
class definitions;
class random_state;

struct context {
    context(context const&) = delete;
    context(context&&) = delete;
    context& operator=(context const&) = delete;
    context& operator=(context&&) = delete;

    random_state&      random;
    definitions const& data;
    output&            out;

    item_factory&     ifactory;
    creature_factory& cfactory;
};

} //namespace bkrl
