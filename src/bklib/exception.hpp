#pragma once

#include <exception>
#include <boost/exception/all.hpp>

namespace bklib {

struct exception_base : virtual std::exception, virtual boost::exception {
    virtual ~exception_base() noexcept;

    exception_base() = default;
    exception_base(exception_base const&) = default;
    exception_base(exception_base&&) = default;
    exception_base& operator=(exception_base const&) = default;
    exception_base& operator=(exception_base&&) = default;
};

struct platform_error : virtual exception_base {
    virtual ~platform_error() noexcept;

    platform_error() = default;
    platform_error(platform_error const&) = default;
    platform_error(platform_error&&) = default;
    platform_error& operator=(platform_error const&) = default;
    platform_error& operator=(platform_error&&) = default;
};

struct io_error : virtual exception_base {
    virtual ~io_error() noexcept;

    io_error() = default;
    io_error(io_error const&) = default;
    io_error(io_error&&) = default;
    io_error& operator=(io_error const&) = default;
    io_error& operator=(io_error&&) = default;
};

} //namespace bklib
