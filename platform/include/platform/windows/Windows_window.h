//
// This file is part of the "platform" project
// See "LICENSE" for license information.
//

#ifndef PLATFORM_WINDOWS_WINDOW_GUARD
#define PLATFORM_WINDOWS_WINDOW_GUARD

#include <string>
#include <sigs.h>
#include <Windows.h>
#include "platform/enums.h"
#include "platform/Extent.h"

namespace Platform {

//----------------------------------------------------------------------------------------------------------------------

template<typename Signature>
using Signal = sigs::Signal<Signature>;

//----------------------------------------------------------------------------------------------------------------------

struct Windows_window_desc {
    std::wstring title;
    Extent extent { 0, 0, 1 };
};

//----------------------------------------------------------------------------------------------------------------------

class Windows_window final {
public:
    Signal<void()> startup_signal;
    Signal<void()> shutdown_signal;
    Signal<void()> render_signal;
    Signal<void(const Extent&)> resize_signal;
    Signal<void(Key)> key_down_signal;
    Signal<void(Key)> key_up_signal;

    Windows_window(const Windows_window_desc& desc);

    ~Windows_window();

    void run();

    inline auto title() const noexcept
    { return title_; }

    Extent extent() const noexcept;

    inline auto window() const noexcept
    { return window_; }

private:
    void init_atom_();

    void init_window_(const Windows_window_desc& desc);

    void term_atom_();

    void term_window_();

private:
    std::wstring title_;
    ATOM atom_;
    HWND window_;
};

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Platform

#endif // PLATFORM_WINDOWS_WINDOW_GUARD
