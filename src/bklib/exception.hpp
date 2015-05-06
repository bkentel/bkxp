#pragma once

#include <exception>
#include <boost/exception/all.hpp>

namespace bklib {

struct exception_base : virtual std::exception, virtual boost::exception { };
struct platform_error : virtual exception_base { };
struct io_error : virtual exception_base { };

} //namespace bklib
