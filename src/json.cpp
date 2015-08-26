#include "json.hpp"

#include "item.hpp"
#include "creature.hpp"
#include "color.hpp"

#include "bklib/exception.hpp"
#include "bklib/assert.hpp"
#include "bklib/scope_guard.hpp"

#include "bklib/dictionary.hpp"

#include <rapidjson/reader.h>

#define BK_HASHED_ENUM(name) name = ::bklib::static_djb2_hash(#name)

namespace {

using size_type = rapidjson::SizeType;
using string_view = bklib::utf8_string_view;
using namespace bklib::literals;

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
template <typename Parser>
void parse_from_string(Parser&& parser, string_view const json) {
    rapidjson::Reader reader;
    rapidjson::StringStream ss {json.data()};

    auto const result = reader.Parse(ss, std::forward<Parser>(parser));
    if (result) {
        return;
    }

    auto const code   = result.Code();
    auto const offset = result.Offset();

    if (code == rapidjson::kParseErrorDocumentRootNotSingular
     && offset == json.size()
    ) {
        return; //TODO hack?
    }

    BOOST_THROW_EXCEPTION(bkrl::json_error {}
      << bkrl::json_error_code   {static_cast<bkrl::json_error_code_type>(code)}
      << bkrl::json_error_offset {offset}
    );
}

//--------------------------------------------------------------------------------------------------
//! Polymorphic base class for all json parsers.
//--------------------------------------------------------------------------------------------------
class parser_base {
public:
    explicit parser_base(parser_base& root, bool const default_result_value = false) noexcept
      : root_ {std::addressof(root)}
      , default_result_ {default_result_value}
    {
    }

    virtual ~parser_base() noexcept { }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Functions required by rapidjson's SAX model.
    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool Null() { return top().on_null(); }

    bool Bool(bool const b) { return top().on_bool(b); }

    bool Int(int const i) { return top().on_int(i); }

    bool Uint(unsigned const u) { return top().on_uint(u); }

    bool Int64(int64_t const i) { return top().on_int64(i); }

    bool Uint64(uint64_t const u) { return top().on_uint64(u);}

    bool Double(double const d) { return top().on_double(d); }

    bool String(const char* const str, size_type const len, bool const copy) {
        return top().on_string(str, len, copy);
    }

    bool StartObject() {
        return top().on_start_object();
    }

    bool Key(const char* const str, size_type const len, bool const copy) {
        return top().on_key(str, len, copy);
    }

    bool EndObject(size_type const size) {
        return top().on_end_object(size);
    }

    bool StartArray() {
        return top().on_start_array();
    }

    bool EndArray(size_type const size) {
        return top().on_end_array(size);
    }

    // an extra overload
    bool Key(string_view const str, uint32_t const hash) {
        return top().on_key(str, hash);
    }
public:
    bool default_result() const noexcept { return default_result_; }
    void activate(parser_base const& old) { on_activate(old); }

    virtual void         push(parser_base& parser)      { root_->push(parser); }
    virtual void         push()                         { root_->push(*this); }
    virtual void         pop(parser_base const& parser) { root_->pop(parser); }
    virtual void         pop()                          { root_->pop(*this); }
    virtual parser_base& top()                          { return root_->top(); }
private:
    virtual void on_activate(parser_base const&) { }
    //----------------------------------------------------------------------------------------------
    virtual bool on_null()                                              { return default_result_; }
    virtual bool on_bool(bool const)                                    { return default_result_; }
    virtual bool on_int(int const)                                      { return default_result_; }
    virtual bool on_uint(unsigned const)                                { return default_result_; }
    virtual bool on_int64(int64_t const)                                { return default_result_; }
    virtual bool on_uint64(uint64_t const)                              { return default_result_; }
    virtual bool on_double(double const)                                { return default_result_; }
    virtual bool on_string(char const*, size_type const, bool const)    { return default_result_; }
    virtual bool on_start_object()                                      { return default_result_; }
    virtual bool on_key(char const* const, size_type const, bool const) { return default_result_; }
    virtual bool on_key(string_view const, uint32_t const)              { return default_result_; }
    virtual bool on_end_object(size_type const)                         { return default_result_; }
    virtual bool on_start_array()                                       { return default_result_; }
    virtual bool on_end_array(size_type const)                          { return default_result_; }
private:
    parser_base* root_;
    bool         default_result_;
};

//--------------------------------------------------------------------------------------------------
//! A specialized parser that acts as a stack of active parsers.
//--------------------------------------------------------------------------------------------------
class root_parser final : public parser_base {
public:
    explicit root_parser(bool const default_result = false)
      : parser_base {*this, default_result}
    {
        stack_.push_back(this);
    }

    void push(parser_base& parser) override final {
        auto const& old = top();

        BK_PRECONDITION(std::addressof(old) != std::addressof(parser));

        stack_.push_back(&parser);
        parser.activate(old);
    }

    void pop(parser_base const& parser) override final {
        auto const& old = top();

        if (std::addressof(old) != std::addressof(parser)) {
            BK_ASSERT(false);
        }

        stack_.pop_back();

        if (!stack_.empty()) {
            stack_.back()->activate(old);
        }
    }

    parser_base& top() override final {
        BK_PRECONDITION(!stack_.empty());
        return *stack_.back();
    }
private:
    std::vector<parser_base*> stack_;
};

//--------------------------------------------------------------------------------------------------
//! Simple parser that accepts all input and does nothing.
//--------------------------------------------------------------------------------------------------
class null_object_parser final : public parser_base {
public:
    explicit null_object_parser(root_parser& root)
      : parser_base {root, true}
    {
    }

    bool on_start_object() override final {
        if (!in_object_) {
            return (in_object_ = true);
        }

        return false;
    }

    bool on_end_object(size_type) override final {
        if (in_object_) {
            pop();
            return !(in_object_ = false);
        }

        return false;
    }
private:
    bool in_object_ = false;
};

//--------------------------------------------------------------------------------------------------
//! Simple pass-through parser to facilitate testing.
//--------------------------------------------------------------------------------------------------
class test_parser final : public parser_base {
public:
    test_parser(root_parser& root, parser_base& parser)
      : parser_base {root}
      , parser_ {std::addressof(parser)}
    {
    }

    bool on_start_array() override final {
        if (state_ != state::none) {
            return false;
        }

        state_ = state::in_array;
        push(*parser_);

        return true;
    }

    bool on_end_array(size_type) override final {
        if (state_ != state::in_array) {
            return false;
        }

        state_ = state::none;
        pop();

        return true;
    }

    bool on_start_object() override final {
        if (state_ != state::none) {
            return false;
        }

        state_ = state::in_object;
        push(*parser_);

        return true;
    }

    bool on_end_object(size_type) override final {
        if (state_ != state::in_object) {
            return false;
        }

        state_ = state::none;
        pop();

        return true;
    }
private:
    parser_base* parser_;
    enum class state {
        none, in_object, in_array
    } state_ = state::none;
};

//--------------------------------------------------------------------------------------------------
//! A parser that expects to read one string value and write it to the destination provided by
//! @c operator().
//--------------------------------------------------------------------------------------------------
template <typename StringType = bklib::utf8_string>
class expect_string_parser final : public parser_base {
public:
    explicit expect_string_parser(root_parser& root)
      : parser_base {root}
    {
    }

    void operator()(StringType& out) {
        out_ = std::addressof(out);
        push();
    }

    //----------------------------------------------------------------------------------------------
    bool on_string(char const* const str, size_type const len, bool) override final {
        BK_PRECONDITION(out_);
        BK_SCOPE_EXIT {
            out_ = nullptr;
            pop();
        };

        out_->assign(str, len);

        return true;
    }
private:
    StringType* out_ = nullptr;
};

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
class definition_parser final : public parser_base {
public:
    using select_t = std::function<parser_base* (string_view)>;
    using finish_t = std::function<bool ()>;

    definition_parser(root_parser& root, select_t on_select, finish_t on_finish)
      : parser_base {root}
      , on_select_ {std::move(on_select)}
      , on_finish_ {std::move(on_finish)}
    {
    }

    void on_activate(parser_base const& old) override final {
        if (field_ == field::definitions) {
            if (std::addressof(old) == definition_parser_) {
                if (on_finish_()) {
                    return;
                }
            }
        }

        field_ = field::none;
    }

    //----------------------------------------------------------------------------------------------
    bool on_string(char const* const str, size_type const len, bool) override final {
        if (field_ == field::file_type) {
            field_ = field::none;

            definition_parser_ = on_select_(string_view {str, len});
            if (definition_parser_) {
                return true;
            }
        }

        return default_result();
    }

    bool on_start_object() override final {
        if (field_ == field::none) {
            return true;
        } else if (field_ == field::definitions) {
            if (definition_parser_) {
                push(*definition_parser_);
                top().StartObject();
                return true;
            }
        }

        return default_result();
    }

    bool on_key(char const* const str, size_type const len, bool) override final {
        string_view const s {str, len};
        return on_key(s, bklib::djb2_hash(s));
    }

    bool on_key(string_view const, uint32_t const hash) override final {
        if (field_ != field::none) {
            return default_result();
        }

        switch (field_ = static_cast<field>(hash)) {
        case field::file_type   :
        case field::definitions :
            break;
        case field::none :
        default :
            return default_result();
        }

        return true;
    }

    bool on_end_object(size_type) override final {
        if (field_ == field::none) {
            return true;
        }

        return default_result();
    }

    bool on_start_array() override final {
        if (field_ == field::definitions) {
            return true;
        }

        return default_result();
    }

    bool on_end_array(size_type) override final {
        if (field_ == field::definitions) {
            field_ = field::none;
            return true;
        }

        return default_result();
    }
private:
    select_t on_select_;
    finish_t on_finish_;

    parser_base* definition_parser_ = nullptr;

    enum class field : uint32_t {
        none
      , BK_HASHED_ENUM(file_type)
      , BK_HASHED_ENUM(definitions)
    } field_ = field::none;
};

//--------------------------------------------------------------------------------------------------
//! A parser for string tags. i.e. An array of 0 or more strings.
//--------------------------------------------------------------------------------------------------
class tags_parser final : public parser_base {
public:
    using tag_it = bkrl::tag_list::const_iterator;
    using finish_t = std::function<bool (tag_it, tag_it, tag_it)>;

    tags_parser(root_parser& root, finish_t on_finish)
      : parser_base {root}
      , on_finish_ {std::move(on_finish)}
    {
    }

    //----------------------------------------------------------------------------------------------
    bool on_string(char const* const str, size_type const len, bool) override final {
        tags_.emplace_back(bklib::utf8_string_view {str, len});
        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_start_array() override final {
        tags_.clear();
        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_end_array(size_type) override final {
        auto const beg = std::begin(tags_);
        auto const end = std::end(tags_);

        std::sort(beg, end);
        auto const uend = std::unique(beg, end);

        auto const result = on_finish_(beg, uend, end);

        pop();

        return result;
    }
private:
    finish_t       on_finish_;
    bkrl::tag_list tags_;
};

//--------------------------------------------------------------------------------------------------
//!
//--------------------------------------------------------------------------------------------------
class definition_base_parser final : public parser_base {
public:
    decltype(auto) make_on_finish() {
        using namespace std::placeholders;
        return std::bind(&definition_base_parser::on_finish_tags, this, _1, _2, _3);
    }

    definition_base_parser(root_parser& root, bkrl::definition_base& out)
      : parser_base {root}
      , string_parser_ {root}
      , tag_parser_ {root, make_on_finish()}
      , out_ {std::addressof(out)}
    {
    }

    bool on_finish_tags(
        tags_parser::tag_it const beg
      , tags_parser::tag_it const uend
      , tags_parser::tag_it const end
    ) {
        for (auto it = uend; it != end; ++it) {
            //TODO
        }

        out_->tags.assign(beg, uend);
        return true;
    }

    void on_activate(parser_base const&) override final {
        field_ = field::none;
    }

    bool on_string(char const* const str, size_type const len, bool const) override final {
        if (field_ == field::symbol_color) {
            field_ = field::none;
            out_->symbol_color.reset(string_view {str, len});
            return true;
        }

        return default_result();
    }

    bool on_key(char const* const str, size_type const len, bool const) override final {
        string_view const key {str, len};
        return on_key(key, bklib::djb2_hash(key));
    }

    bool on_key(string_view const str, uint32_t const hash) override final {
        BK_ASSERT(field_ == field::none);

        switch (field_ = static_cast<field>(hash)) {
        case field::id           : string_parser_(out_->id_string);   break;
        case field::name         : string_parser_(out_->name);        break;
        case field::description  : string_parser_(out_->description); break;
        case field::symbol       : string_parser_(out_->symbol);      break;
        case field::symbol_color : break;
        case field::tags         : push(tag_parser_); break;
        case field::none         : return default_result();
        default:
            field_ = field::none;
            pop();
            return top().Key(str, hash);
        }

        return true;
    }

    bool on_start_object() override final {
        out_->id_string.clear();
        out_->name.clear();
        out_->description.clear();
        out_->symbol.clear();
        out_->symbol_color.reset("");
        out_->tags.clear();

        return true;
    }

    bool on_end_object(size_type const size) override final {
        pop();
        return top().EndObject(size);
    }
private:
    enum class field : uint32_t {
        none
      , BK_HASHED_ENUM(id)
      , BK_HASHED_ENUM(name)
      , BK_HASHED_ENUM(description)
      , BK_HASHED_ENUM(symbol)
      , BK_HASHED_ENUM(symbol_color)
      , BK_HASHED_ENUM(tags)
    } field_ = field::none;

    expect_string_parser<> string_parser_;
    tags_parser            tag_parser_;
    bkrl::definition_base* out_;
};

//----------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------
class item_def_parser final : public parser_base {
public:
    explicit item_def_parser(root_parser& root)
      : parser_base {root}
      , base_parser_ {root, def_}
    {
    }

    void on_activate(parser_base const&) override final {
        field_ = field::none;
    }

    bool on_key(string_view const, uint32_t const hash) override final {
        BK_PRECONDITION(field_ == field::none);

        switch (field_ = static_cast<field>(hash)) {
        case field::weight : break;
        case field::none : BK_FALLTHROUGH
        default:
            return default_result();
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_key(const char* const str, size_type const len, bool const) override final {
        string_view const s {str, len};
        return on_key(s, bklib::djb2_hash(s));
    }

    //----------------------------------------------------------------------------------------------
    bool on_uint(unsigned const n) override final {
        if (field_ != field::weight) {
            return default_result();
        }

        using weight_t = decltype(bkrl::item_def::weight);

        constexpr auto const max = static_cast<unsigned>(std::numeric_limits<weight_t>::max());
        if (n > max) {
            return default_result(); //TODO
        }

        def_.weight = static_cast<weight_t>(n & max);

        field_ = field::none;
        push(base_parser_);

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_start_object() override final {
        push(base_parser_);
        return base_parser_.StartObject();
    }

    //----------------------------------------------------------------------------------------------
    bool on_end_object(size_type const) override final {
        def_.id.reset(def_.id_string);

        pop();
        return true;
    }

    //----------------------------------------------------------------------------------------------
    bkrl::item_def const& result() const {
        return def_;
    }
private:
    definition_base_parser base_parser_;

    bkrl::item_def def_ {""};

    enum class field : uint32_t {
        none
      , BK_HASHED_ENUM(weight)
    } field_ = field::none;
};

//----------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------
class creature_def_parser final : public parser_base {
public:
    explicit creature_def_parser(root_parser& root)
      : parser_base {root}
      , base_parser_ {root, def_}
    {
    }

    void on_activate(parser_base const&) override final {
        field_ = field::none;
    }

    bool on_key(string_view const, uint32_t const hash) override final {
        BK_PRECONDITION(field_ == field::none);

        switch (field_ = static_cast<field>(hash)) {
        case field::stat_hp : break;
        case field::none : BK_FALLTHROUGH
        default:
            return default_result();
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_key(const char* const str, size_type const len, bool const) override final {
        string_view const s {str, len};
        return on_key(s, bklib::djb2_hash(s));
    }

    //----------------------------------------------------------------------------------------------
    bool on_string(char const* const str, size_type const len, bool const) override final {
        if (field_ == field::stat_hp) {
            def_.stat_hp = bkrl::make_random_integer(string_view {str, len});

            field_ = field::none;
            push(base_parser_);

            return true;
        }

        return default_result();
    }

    //----------------------------------------------------------------------------------------------
    bool on_start_object() override final {
        push(base_parser_);
        return base_parser_.StartObject();
    }

    //----------------------------------------------------------------------------------------------
    bool on_end_object(size_type const) override final {
        def_.id.reset(def_.id_string);

        pop();
        return true;
    }

    //----------------------------------------------------------------------------------------------
    bkrl::creature_def const& result() const {
        return def_;
    }
private:
    definition_base_parser base_parser_;

    bkrl::creature_def def_ {""};

    enum class field : uint32_t {
        none
      , BK_HASHED_ENUM(stat_hp)
    } field_ = field::none;
};

//----------------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------------
class color_def_parser final : public parser_base {
public:
    explicit color_def_parser(root_parser& root)
      : parser_base {root}
      , string_parser_ {root}
    {
    }

    void on_activate(parser_base const&) override final {
        field_ = field::none;
    }

    bool on_start_array() override final {
        if (field_ != field::value) {
            return default_result();
        }

        index_ = 0;
        return true;
    }

    bool on_uint(unsigned const n) override final {
        if (field_ != field::value) {
            return default_result();
        }

        if (index_ >= 4) {
            return default_result();
        }

        constexpr auto const max = unsigned {0xFF};
        if (n > max) {
            return default_result();
        }

        color_[index_++] = static_cast<uint8_t>(n & max);
        return true;
    }

    bool on_end_array(size_type) override final {
        if (field_ != field::value) {
            return default_result();
        }

        field_ = field::none;
        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_key(char const* const str, size_type const len, bool const) override final {
        string_view const s {str, len};
        return on_key(s, bklib::djb2_hash(s));
    }

    bool on_key(string_view const, uint32_t const hash) override final {
        BK_ASSERT(field_ == field::none);

        switch (field_ = static_cast<field>(hash)) {
        case field::id:         string_parser_(id_);         break;
        case field::short_name: string_parser_(short_name_); break;
        case field::value:      break;
        case field::none:       BK_FALLTHROUGH
        default:
            field_ = field::none;
            return default_result();
        }

        return true;
    }

    bool on_start_object() override final {
        id_.clear();
        short_name_.clear();
        color_.fill(0xFF);

        return true;
    }

    //----------------------------------------------------------------------------------------------
    bool on_end_object(size_type const) override final {
        pop();
        return true;
    }

    bkrl::color_def result() const {
        bkrl::color_def def {id_};
        def.short_name = short_name_;
        def.color = color_;
        return def;
    }
private:
    enum class field : uint32_t {
        none
      , BK_HASHED_ENUM(id)
      , BK_HASHED_ENUM(short_name)
      , BK_HASHED_ENUM(value)
    } field_ = field::none;

    unsigned index_ = 0;

    expect_string_parser<> string_parser_;

    bklib::utf8_string id_;
    bklib::utf8_string short_name_;
    bkrl::color4       color_;
};

template <typename Parser, typename Callback>
int load_definitions_helper(
    string_view const data
  , string_view const expected_file_type
  , Callback&& callback
) {
    root_parser root {};
    Parser parser {root};

    int count {0};

    definition_parser def_parser {root
      , [&](string_view const file_type) -> parser_base* {
            return file_type == expected_file_type ? std::addressof(parser) : nullptr;
        }
      , [&] {
            return callback(parser) ?
                (++count, true) : false;
        }
    };

    root.push(def_parser);

    try {
        parse_from_string(root, data);
    } catch (bkrl::json_error& e) {
        e << bkrl::json_error_success_count {count};
        throw;
    }

    return count;
}

} //namespace

bkrl::json_error::~json_error() noexcept = default;

namespace {
constexpr char const* const creatures_file_type {"creatures"};
constexpr char const* const items_file_type     {"items"};
constexpr char const* const colors_file_type    {"colors"};

} //namespace

int bkrl::load_definitions(
    bklib::utf8_string_view const data
  , std::function<bool (item_def const&)> callback
) {
    return load_definitions_helper<item_def_parser>(data, items_file_type, [&](auto&& parser) {
        return callback(parser.result());
    });
}

int bkrl::load_definitions(
    bklib::utf8_string_view const data
  , std::function<bool (creature_def const&)> callback
) {
    return load_definitions_helper<creature_def_parser>(data, creatures_file_type, [&](auto&& parser) {
        return callback(parser.result());
    });
}

int bkrl::load_definitions(
    bklib::utf8_string_view const data
  , std::function<bool (color_def const&)> callback
) {
    return load_definitions_helper<color_def_parser>(data, colors_file_type, [&](auto&& parser) {
        return callback(parser.result());
    });
}

int bkrl::load_definitions(bklib::dictionary<creature_def>& dic, bklib::utf8_string_view const data, load_from_string_t)
{
    return load_definitions_helper<creature_def_parser>(data, creatures_file_type, [&](auto&& parser) {
        auto const result = dic.insert_or_replace(parser.result());
        process_tags(*result.first);
        return true;
    });
}

int bkrl::load_definitions(bklib::dictionary<item_def>& dic, bklib::utf8_string_view const data, load_from_string_t)
{
    return load_definitions_helper<item_def_parser>(data, items_file_type, [&](auto&& parser) {
        auto const result = dic.insert_or_replace(parser.result());
        process_tags(*result.first);
        return true;
    });
}

int bkrl::load_definitions(bklib::dictionary<color_def>& dic, bklib::utf8_string_view const data, load_from_string_t)
{
    return load_definitions_helper<color_def_parser>(data, colors_file_type, [&](auto&& parser) {
        dic.insert_or_replace(parser.result());
        return true;
    });
}

//--------------------------------------------------------------------------------------------------
namespace {
template <typename T>
inline int load_definitions_from_file(
    bklib::dictionary<T>& dic
  , bklib::utf8_string_view const filename
) {
    auto const buf = bklib::read_file_to_buffer(filename);
    return bkrl::load_definitions(dic, bklib::utf8_string_view {buf.data(), buf.size()}, bkrl::load_from_string);
}
} //namespace

int bkrl::load_definitions(bklib::dictionary<creature_def>& dic, bklib::utf8_string_view const filename, load_from_file_t)
{
    return load_definitions_from_file(dic, filename);
}

int bkrl::load_definitions(bklib::dictionary<item_def>& dic, bklib::utf8_string_view const filename, load_from_file_t)
{
    return load_definitions_from_file(dic, filename);
}

int bkrl::load_definitions(bklib::dictionary<color_def>& dic, bklib::utf8_string_view const filename, load_from_file_t)
{
    return load_definitions_from_file(dic, filename);
}
