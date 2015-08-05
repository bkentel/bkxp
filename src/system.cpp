#include "system.hpp"
#include "system_sdl.hpp"

#if defined(BK_NO_SDL)
class bkrl::detail::system_impl {
public:
    explicit system_impl(system*) { }

    int client_width() const { return 0; }
    int client_height() const { return 0; }
    void quit() { }
    bool is_running() const noexcept  { return true; }
    void do_events(bool const)  { }
    void delay(std::chrono::nanoseconds const) { }
    key_mod_state current_key_mods() const { return {}; }
};
#endif

bkrl::system::system()
  : impl_ {std::make_unique<detail::system_impl>(this)}
{
}

bkrl::system::~system() = default;

//----------------------------------------------------------------------------------------------
int bkrl::system::client_width() const
{
    return impl_->client_width();
}

//----------------------------------------------------------------------------------------------
int bkrl::system::client_height() const
{
    return impl_->client_height();
}

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

//----------------------------------------------------------------------------------------------
bkrl::key_mod_state bkrl::system::current_key_mods() const {
    return impl_->current_key_mods();
}
