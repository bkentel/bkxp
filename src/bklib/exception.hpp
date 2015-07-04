#pragma once

#include <exception>
#include <boost/exception/all.hpp>

namespace bklib {

struct exception_base : virtual std::exception, virtual boost::exception { virtual ~exception_base() noexcept; };
struct platform_error : virtual exception_base { virtual ~platform_error() noexcept; };
struct io_error : virtual exception_base { virtual ~io_error() noexcept; };

} //namespace bklib
