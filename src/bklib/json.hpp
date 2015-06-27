#pragma once

#include "string.hpp"
#include <rapidjson/reader.h>

namespace bklib {

//--------------------------------------------------------------------------------------------------
//! Base class for rapidjson parsers.
//--------------------------------------------------------------------------------------------------
struct json_parser_base {
    using size_type = rapidjson::SizeType;

    //----------------------------------------------------------------------------------------------
    explicit json_parser_base(json_parser_base* const parent_parser = nullptr)
      : parent {parent_parser}
    {
    }

    virtual ~json_parser_base() noexcept;

    //----------------------------------------------------------------------------------------------
    bool Null() {
        return get_handler()->on_null();
    }

    bool Bool(bool const b) {
        return get_handler()->on_bool(b);
    }

    bool Int(int const i) {
        return get_handler()->on_int(i);
    }

    bool Uint(unsigned const u) {
        return get_handler()->on_uint(u);
    }

    bool Int64(int64_t const i) {
        return get_handler()->on_int64(i);
    }

    bool Uint64(uint64_t const u) {
        return get_handler()->on_uint64(u);
    }

    bool Double(double const d) {
        return get_handler()->on_double(d);
    }

    bool String(const char* const str, size_type const len, bool const copy) {
        return get_handler()->on_string(str, len, copy);
    }

    bool StartObject() {
        return get_handler()->on_start_object();
    }

    bool Key(const char* const str, size_type const len, bool const copy) {
        return get_handler()->on_key(str, len, copy);
    }

    bool EndObject(size_type const size) {
        return get_handler()->on_end_object(size);
    }

    bool StartArray() {
        return get_handler()->on_start_array();
    }

    bool EndArray(size_type const size) {
        return get_handler()->on_end_array(size);
    }

    virtual bool on_finished() { return true; }

    json_parser_base* handler    {this};  //!< handler to use
    json_parser_base* parent     {};      //!< parser that owns / uses this; used by on_finished.
    bool              def_result {false}; //!< the default value to use for unhandled data / state.
private:
    //----------------------------------------------------------------------------------------------
    virtual bool on_null()                               { return def_result; }
    virtual bool on_bool(bool)                           { return def_result; }
    virtual bool on_int(int)                             { return def_result; }
    virtual bool on_uint(unsigned)                       { return def_result; }
    virtual bool on_int64(int64_t)                       { return def_result; }
    virtual bool on_uint64(uint64_t)                     { return def_result; }
    virtual bool on_double(double)                       { return def_result; }
    virtual bool on_string(const char*, size_type, bool) { return def_result; }
    virtual bool on_start_object()                       { return def_result; }
    virtual bool on_key(const char*, size_type, bool)    { return def_result; }
    virtual bool on_end_object(size_type)                { return def_result; }
    virtual bool on_start_array()                        { return def_result; }
    virtual bool on_end_array(size_type)                 { return def_result; }

    //----------------------------------------------------------------------------------------------
    //! Find the deepest handler type.
    //----------------------------------------------------------------------------------------------
    json_parser_base* get_handler() const {
        auto result = handler;
        while (result && result != result->handler) {
            result = result->handler;
        }

        return result;
    }
};

//--------------------------------------------------------------------------------------------------
//! Simple parser that expects to read one string only.
//--------------------------------------------------------------------------------------------------
struct json_string_parser final : public json_parser_base {
    using json_parser_base::json_parser_base;

    virtual ~json_string_parser() noexcept;

    //----------------------------------------------------------------------------------------------
    bool on_string(const char* const str, size_type const len, bool) override final {
        if (out) {
            out->assign(str, len);
        }

        if (parent) {
            return parent->on_finished();
        }

        return true;
    }

    utf8_string* out {}; //!< output destination for the read string.
};

} //namespace bklib
