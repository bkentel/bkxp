#include "system.hpp"
#include "system_sdl.hpp"

bkrl::system::system()
  : impl_ {std::make_unique<detail::system_impl>(this)}
{
}

bkrl::system::~system() = default;

//----------------------------------------------------------------------------------------------
void bkrl::system::quit() {
    impl_->quit();
}

//----------------------------------------------------------------------------------------------
bool bkrl::system::is_running() const noexcept {
    return impl_->is_running();
}

//----------------------------------------------------------------------------------------------
void bkrl::system::do_events_nowait() {
    impl_->do_events(false);
}

//----------------------------------------------------------------------------------------------
void bkrl::system::do_events_wait() {
    impl_->do_events(true);
}
    
//----------------------------------------------------------------------------------------------
void bkrl::system::delay(std::chrono::nanoseconds const ns)
{
    impl_->delay(ns);
}
