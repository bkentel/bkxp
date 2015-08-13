#include "system.hpp"

namespace bkrl { class system_stub_impl; }

class bkrl::system_stub_impl final : public system {
public:
    virtual ~system_stub_impl();

    int client_width() const noexcept override final { return 0; }
    int client_height() const noexcept override final { return 0; }
    void quit() override final { }
    bool is_running() const noexcept override final { return true; }
    void do_events_nowait() override final { }
    void do_events_wait() override final { }
    void delay(std::chrono::nanoseconds) override final { }
    key_mod_state current_key_mods() const noexcept override final { return {}; }
};

bkrl::system_stub_impl::~system_stub_impl() {
}

bkrl::system::~system() {
}

#if defined(BK_NO_SDL)
std::unique_ptr<bkrl::system> bkrl::make_system() {
    return std::make_unique<system_stub_impl>();
}
#endif
