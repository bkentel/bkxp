#pragma once

#include <rapidjson/reader.h>

namespace bklib {

struct json_parser_base {
    using size_type = rapidjson::SizeType;

    //----------------------------------------------------------------------------------------------
    bool Null() {
        return handler->on_null();
    }
    
    bool Bool(bool const b) {
        return handler->on_bool(b);
    }
    
    bool Int(int const i) {
        return handler->on_int(i);
    }
    
    bool Uint(unsigned const u) {
        return handler->on_uint(u);
    }
    
    bool Int64(int64_t const i) {
        return handler->on_int64(i);
    }

    bool Uint64(uint64_t const u) {
        return handler->on_uint64(u);
    }

    bool Double(double const d) {
        return handler->on_double(d);
    }

    bool String(const char* const str, size_type const len, bool const copy) {
        return handler->on_string(str, len, copy);
    }

    bool StartObject() {
        return handler->on_start_object();
    }

    bool Key(const char* const str, size_type const len, bool const copy) {
        return handler->on_key(str, len, copy);
    }

    bool EndObject(size_type const size) {
        return handler->on_end_object(size);
    }

    bool StartArray() {
        return handler->on_start_array();
    }

    bool EndArray(size_type const size) {
        return handler->on_end_array(size);
    }

    //----------------------------------------------------------------------------------------------
    virtual bool on_null()                               { return false; }
    virtual bool on_bool(bool)                           { return false; }
    virtual bool on_int(int)                             { return false; }
    virtual bool on_uint(unsigned)                       { return false; }
    virtual bool on_int64(int64_t)                       { return false; }
    virtual bool on_uint64(uint64_t)                     { return false; }
    virtual bool on_double(double)                       { return false; }
    virtual bool on_string(const char*, size_type, bool) { return false; }
    virtual bool on_start_object()                       { return false; }
    virtual bool on_key(const char*, size_type, bool)    { return false; }
    virtual bool on_end_object(size_type)                { return false; }
    virtual bool on_start_array()                        { return false; }
    virtual bool on_end_array(size_type)                 { return false; }

    //----------------------------------------------------------------------------------------------
    explicit json_parser_base(json_parser_base* const parent = nullptr)
      : parent {parent}
    {
    }

    json_parser_base* handler = this;
    json_parser_base* parent  = nullptr;
};

} //namespace bklib
