//
// This file is part of the "platform" project
// See "LICENSE" for license information.
//

#include "windows/Windows_window.h"
#include <combaseapi.h>

using namespace std;
using namespace Platform;

namespace {

//----------------------------------------------------------------------------------------------------------------------

inline std::wstring unique_class_name()
{
    GUID guid {};
    auto result = CoCreateGuid(&guid);

    assert(result == S_OK);
    wstring class_name { L"{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}~" };
    auto length = StringFromGUID2(guid, &class_name[0], static_cast<int>(class_name.size()));

    assert(length == class_name.size());
    return class_name;
}

//----------------------------------------------------------------------------------------------------------------------

LRESULT CALLBACK window_proc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto window = reinterpret_cast<Windows_window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (uMsg) {
        case WM_PAINT:
            window->render_signal();
            return 0;
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        case WM_DESTROY:
            window->shutdown_signal();
            return 0;
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}

//----------------------------------------------------------------------------------------------------------------------

} // of namespace

namespace Platform {

//----------------------------------------------------------------------------------------------------------------------

Windows_window::Windows_window(const Windows_window_desc& desc) :
    title_ {desc.title},
    atom_ {NULL},
    window_{NULL}
{
    init_atom_();
    init_window_(desc);
}

//----------------------------------------------------------------------------------------------------------------------

Windows_window::~Windows_window()
{
    term_window_();
    term_atom_();
}

//----------------------------------------------------------------------------------------------------------------------

void Windows_window::run()
{
    startup_signal();
    ShowWindow(window_, SW_SHOW);

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

//----------------------------------------------------------------------------------------------------------------------

Extent Windows_window::extent() const noexcept
{
    RECT rect;

    GetClientRect(window_, &rect);

    return { static_cast<uint32_t>(rect.right - rect.left), static_cast<uint32_t>(rect.bottom - rect.top), 1 };
}

//----------------------------------------------------------------------------------------------------------------------

void Windows_window::init_atom_()
{
    auto class_name = unique_class_name();

    if (class_name.empty())
        throw runtime_error("fail to create a window");

    WNDCLASSEX wnd_class{
        sizeof(WNDCLASSEX),
        CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        window_proc,
        0,
        0,
        GetModuleHandle(NULL),
        LoadIcon(nullptr, IDI_APPLICATION),
        LoadCursor(nullptr, IDC_ARROW),
        nullptr,
        nullptr,
        &class_name[0],
        nullptr,
    };

    atom_ = RegisterClassEx(&wnd_class);

    if (!atom_)
        throw runtime_error("fail to create a window");
}

//----------------------------------------------------------------------------------------------------------------------

void Windows_window::init_window_(const Windows_window_desc& desc)
{
    RECT rect { 0, 0, static_cast<LONG>(desc.extent.w), static_cast<LONG>(desc.extent.h) };
    
    if (!AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0))
        throw runtime_error("fail to create a window");

    window_ = CreateWindowEx(0, MAKEINTATOM(atom_),
                             &desc.title[0],
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             rect.right - rect.left,
                             rect.bottom - rect.top,
                             nullptr, nullptr,
                             GetModuleHandle(NULL),
                             nullptr);

    if (!window_)
        throw runtime_error("fail to create a window");

    if (SetWindowLongPtr(window_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this)))
        throw runtime_error("fail to create a window");
}

//----------------------------------------------------------------------------------------------------------------------

void Windows_window::term_atom_()
{
    UnregisterClass(MAKEINTATOM(atom_), GetModuleHandle(NULL));
}

//----------------------------------------------------------------------------------------------------------------------

void Windows_window::term_window_()
{
    DestroyWindow(window_);
}

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Platform
