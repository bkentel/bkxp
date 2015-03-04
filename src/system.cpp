#include "system.hpp"
#include "system_sdl.hpp"

bkrl::system::system()
  : impl_ {std::make_unique<detail::system_impl>(this)}
{
}

bkrl::system::~system() = default;

//----------------------------------------------------------------------------------------------
void bkrl::system::do_events(bool const wait)
{
    impl_->do_events(wait);
}
    
//----------------------------------------------------------------------------------------------
void bkrl::system::delay(std::chrono::nanoseconds const ns)
{
    impl_->delay(ns);
}
