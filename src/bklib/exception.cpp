#include "exception.hpp"

bklib::exception_base::~exception_base() noexcept = default;
bklib::platform_error::~platform_error() noexcept = default;
bklib::io_error::~io_error() noexcept = default;
