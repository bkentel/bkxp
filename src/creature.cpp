#include "creature.hpp"
#include "renderer.hpp"
#include "map.hpp"

#include "bklib/json.hpp"

#include <unordered_map>
#include <fstream>
#include <functional>

struct creature_def_parser final : bklib::json_parser_base {
    using json_parser_base::json_parser_base;

    enum class state_type {
        begin_def, in_def, end_def
    };

    //----------------------------------------------------------------------------------------------
    bool on_string(const char* const str, size_type const len, bool) override final {       
        if (!string_out) {
            return false;
        }

        (this->*string_out).assign(str, len);
        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_start_object() override final {
        if (state != state_type::begin_def) {
            return false;
        }
        
        state = state_type::in_def;
        return true;
    }
    
    //----------------------------------------------------------------------------------------------
    bool on_key(const char* const str, size_type const len, bool) override final {               
        using key_t   = decltype(bklib::static_djb2_hash(""));
        using value_t = bklib::utf8_string creature_def_parser::*;

        static auto const make_pair = [](auto const str, auto const out) {
            return std::make_pair(bklib::static_djb2_hash(str), out);
        };

        static std::unordered_map<key_t, value_t> const keys {
            make_pair("id",           &creature_def_parser::id)
          , make_pair("name",         &creature_def_parser::name)
          , make_pair("description",  &creature_def_parser::description)
          , make_pair("symbol",       &creature_def_parser::symbol)
          , make_pair("symbol_color", &creature_def_parser::symbol_color)
        };

        if (state != state_type::in_def) {
            return false;
        }              

        auto const key_hash = bklib::djb2_hash(str, str + len);
        auto const it = keys.find(key_hash);
        if (it == end(keys)) {
            return false;
        }

        string_out = it->second;
        return true;
    }
    
    //----------------------------------------------------------------------------------------------
    bool on_end_object(size_type const size) override final {
        state = state_type::begin_def;
        return parent->on_end_object(size);
    }

    //----------------------------------------------------------------------------------------------
    bool on_end_array(size_type const size) override final {
        state = state_type::end_def;
        return parent->on_end_array(size);
    }

    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------
    state_type state = state_type::begin_def;
    
    bklib::utf8_string creature_def_parser::* string_out = nullptr;

    bklib::utf8_string id;
    bklib::utf8_string name;
    bklib::utf8_string description;
    bklib::utf8_string symbol;
    bklib::utf8_string symbol_color;
};

struct creature_defs_parser final : bklib::json_parser_base {
    enum class state_type {
        begin, file_type, definitions, definitions_parse, end
    };

    //----------------------------------------------------------------------------------------------
    bool on_string(const char* const str, size_type const len, bool const copy) override final {
        bklib::utf8_string_view const value {str, len};

        switch (state) {
        case state_type::file_type:
            if (value == "creatures") {
                state = state_type::definitions;
                return true;
            }
            break;
        }

        return false;
    }

    //----------------------------------------------------------------------------------------------
    bool on_start_object() override {
        switch (state) {
        case state_type::begin:
            state = state_type::file_type;
            return true;
        }

        return false;
    }
    
    //----------------------------------------------------------------------------------------------
    bool on_key(const char* const str, size_type const len, bool) override final {
        bklib::utf8_string_view const key {str, len};

        switch (state) {
        case state_type::file_type:
            if (key == "file_type") {
                return true;
            }
            break;
        case state_type::definitions:
            if (key == "definitions") {
                return true;
            }
            break;
        }

        return false;
    }
    
    //----------------------------------------------------------------------------------------------
    bool on_start_array() override final {
        if (state != state_type::definitions) {
            return false;
        }

        state = state_type::definitions_parse;
        handler = &def_parser;

        return true;
    }
    
    //----------------------------------------------------------------------------------------------
    bool on_end_array(size_type const size) override final {
        if (state == state_type::definitions_parse) {
            state = state_type::end;
            handler = this;
            return true;
        }

        return false;
    }

    //----------------------------------------------------------------------------------------------
    bool on_end_object(size_type const size) override final {
        if (state == state_type::end) {
            return true;
        } else if (state != state_type::definitions_parse) {
            return false;
        }

        bkrl::creature_def def {std::move(def_parser.id)};
        def.name         = std::move(def_parser.name);
        def.description  = std::move(def_parser.description);
        def.symbol       = std::move(def_parser.symbol);
        def.symbol_color = std::move(def_parser.symbol_color);

        on_finish(std::move(def));

        return true;
    }

    //----------------------------------------------------------------------------------------------
    //----------------------------------------------------------------------------------------------
    std::function<void (bkrl::creature_def&&)> on_finish;

    state_type state = state_type::begin;
    creature_def_parser def_parser {this};
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::creature
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
void bkrl::creature::draw(renderer& render) const
{
    render.draw_cell(x(pos_), y(pos_), 1);
}

//--------------------------------------------------------------------------------------------------
void bkrl::creature::advance(random_state& random, map& m)
{
    auto& rnd = random[random_stream::creature];
    
    if (!x_in_y_chance(rnd, 1, 3)) {
        return;
    }

    int const dx = random_range(rnd, -1, 1);
    int const dy = random_range(rnd, -1, 1);

    m.move_creature_by(*this, bklib::ivec2 {dx, dy});
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::is_player() const noexcept
{
    return flags_.test(creature_flag::is_player);
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::move_by(bklib::ivec2 const v)
{
    pos_ += v;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::move_by(int const dx, int const dy)
{
    return move_by(bklib::ivec2 {dx, dy});
}

//--------------------------------------------------------------------------------------------------
bklib::ipoint2 bkrl::creature::position() const noexcept
{
    return pos_;
}

//--------------------------------------------------------------------------------------------------
bkrl::creature_instance_id bkrl::creature::id() const noexcept
{
    return id_;
}

//--------------------------------------------------------------------------------------------------
bkrl::creature_def_id bkrl::creature::def() const noexcept
{
    return def_;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::can_get_items(item_pile const& ip) const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
bool bkrl::creature::can_get_item(item const& i) const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
void bkrl::creature::get_item(item&& i)
{
    items_.insert(std::move(i));
}

//--------------------------------------------------------------------------------------------------
void bkrl::creature::get_items(item_pile&& ip)
{
    items_.insert(std::move(ip));
}

//--------------------------------------------------------------------------------------------------
bkrl::creature::creature(
    creature_instance_id const  id
  , creature_def         const& def
  , bklib::ipoint2       const  p
) : id_  {id}
  , def_ {def.id}
  , pos_ {p}
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::creature_factory
////////////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
bkrl::creature bkrl::creature_factory::create(
    random_state&        random
  , creature_def const&  def
  , bklib::ipoint2 const p
) {
    return creature {creature_instance_id {++next_id_}, def, p};
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// bkrl::creature_dictionary
////////////////////////////////////////////////////////////////////////////////////////////////////
class bkrl::detail::creature_dictionary_impl {
public:
    explicit creature_dictionary_impl(bklib::utf8_string_view const filename);
    
    creature_def const* operator[](creature_def_id id) const;
    creature_def const* operator[](uint32_t hash) const;
private:
    std::vector<creature_def> defs_;
};

//--------------------------------------------------------------------------------------------------
std::vector<char> read_file_to_buffer(bklib::utf8_string_view const filename)
{
    std::ifstream file {filename.data(), std::ios::binary};
    if (!file) {
        BK_ASSERT(false);
    }

    file.seekg(0, std::ios::end);
    std::streamsize const size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> result(size);
    if (!file.read(result.data(), size)) {
        BK_ASSERT(false);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
bkrl::detail::creature_dictionary_impl::creature_dictionary_impl(
    bklib::utf8_string_view const filename
)
{
    auto const buffer = read_file_to_buffer("./data/creatures.def");
    rapidjson::Reader reader;
    creature_defs_parser handler;
    
    handler.on_finish = [&](creature_def&& cdef) {
        defs_.push_back(std::move(cdef));
    };

    rapidjson::StringStream ss(buffer.data());
    reader.Parse(ss, handler);   
}

//--------------------------------------------------------------------------------------------------
bkrl::creature_def const*
bkrl::detail::creature_dictionary_impl::operator[](creature_def_id const id) const
{
    return bklib::find_maybe(defs_, [&](creature_def const& def) {
        return def.id == id;
    });
}

//--------------------------------------------------------------------------------------------------
bkrl::creature_def const*
bkrl::detail::creature_dictionary_impl::operator[](uint32_t const id) const
{
    return (*this)[creature_def_id {id}];
}

//--------------------------------------------------------------------------------------------------
bkrl::creature_dictionary::~creature_dictionary() = default;

//--------------------------------------------------------------------------------------------------
bkrl::creature_dictionary::creature_dictionary(bklib::utf8_string_view const filename)
  : impl_ {std::make_unique<detail::creature_dictionary_impl>(filename)}
{
}

//--------------------------------------------------------------------------------------------------
bkrl::creature_def const*
bkrl::creature_dictionary::operator[](creature_def_id const id) const
{
    return (*impl_)[id];
}

//--------------------------------------------------------------------------------------------------
bkrl::creature_def const*
bkrl::creature_dictionary::operator[](uint32_t const hash) const
{
    return (*impl_)[hash];
}
