//
// This file is part of the "platform" project
// See "LICENSE" for license information.
//

#ifndef PLATFORM_MAC_WINDOW_GUARD
#define PLATFORM_MAC_WINDOW_GUARD

#include <string>
#include <Cocoa/Cocoa.h>
#include <sigs.h>
#include "platform/enums.h"
#include "platform/Extent.h"

namespace Platform {

//----------------------------------------------------------------------------------------------------------------------

template<typename Signature>
using Signal = sigs::Signal<Signature>;

//----------------------------------------------------------------------------------------------------------------------

struct Osx_window_desc {
    std::wstring title;
    Extent extent { 0, 0, 1 };
    bool resizable { false };
};

//----------------------------------------------------------------------------------------------------------------------

class Osx_window final {
public:
    Signal<void()> startup_signal;
    Signal<void()> shutdown_signal;
    Signal<void()> render_signal;
    Signal<void(const Extent&)> resize_signal;
    Signal<void()> touch_down_signal;
    Signal<void(float, float)> touch_move_signal;
    Signal<void()> touch_up_signal;
    Signal<void(Key)> key_down_signal;
    Signal<void(Key)> key_up_signal;

    explicit Osx_window(const Osx_window_desc& desc);

    void run();

    void on_startup();

    void on_shutdown();

    void on_render();

    void on_resize(const Extent& extent);

    void on_key_down(Key key);

    void on_key_up(Key key);

    inline auto title() const noexcept
    { return title_; }

    inline auto extent() const noexcept
    { return extent_; }

    inline auto window() const noexcept
    { return window_; }

private:
    void init_app_();

    void init_window_();

private:
    std::wstring title_;
    Extent extent_;
    NSWindowStyleMask style_mask_;
    NSWindow* window_;
};

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Platform

#endif // PLATFORM_MAC_WINDOW_GUARD
