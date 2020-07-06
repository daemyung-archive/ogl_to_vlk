//
// This file is part of the "platform" project
// See "LICENSE" for license information.
//

#include "android/Android_window.h"

using namespace std;
using namespace Platform;

namespace {

//----------------------------------------------------------------------------------------------------------------------

void init_buffers_geometry(const Android_window* window)
{
    ANativeWindow_setBuffersGeometry( window->window(),
                                      window->extent().w, window->extent().h,
                                      ANativeWindow_getFormat(window->window()));
}

//----------------------------------------------------------------------------------------------------------------------

void on_app_cmd(android_app* app, int cmd)
{
    auto window = static_cast<Android_window*>(app->userData);

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            init_buffers_geometry(window);
            window->on_startup();
            break;
        case APP_CMD_TERM_WINDOW:
            window->on_shutdown();
            break;
        default:
            break;
    }
}

//----------------------------------------------------------------------------------------------------------------------

int32_t on_input_event(android_app* app, AInputEvent* event)
{
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION)
        return 0;

    auto window = static_cast<Android_window*>(app->userData);

    switch (AMotionEvent_getAction(event)) {
        case AKEY_EVENT_ACTION_DOWN:
            window->touch_down_signal();
            break;
        case AKEY_EVENT_ACTION_UP:
            window->touch_up_signal();
            break;
        default:
            window->touch_move_signal(AMotionEvent_getX(event, 0), AMotionEvent_getY(event, 0));
            break;
    }

    return 1;
}

//----------------------------------------------------------------------------------------------------------------------

} // of namespace

namespace Platform {

//----------------------------------------------------------------------------------------------------------------------

Android_window::Android_window(const Android_window_desc& desc) :
    title_ { desc.title },
    extent_ { desc.extent },
    app_ { desc.app },
    running_ { false }
{
    init_app_();
}

void Android_window::run()
{
    int events;
    android_poll_source* source;

    do {
        if (ALooper_pollAll(running_ ? 0 : -1, nullptr, &events, (void**)&source) >= 0) {
            if (source)
                source->process(app_, source);
        }

        if (running_)
            render_signal();

    } while (!app_->destroyRequested);
}

void Android_window::on_startup()
{
    running_ = true;
    startup_signal();
}

void Android_window::on_shutdown()
{
    shutdown_signal();
    running_ = false;
}

void Android_window::on_resize(const Extent& extent)
{
    extent_ = extent;
    resize_signal(extent);
}

void Android_window::on_key_down(Key key)
{
    key_down_signal(move(key));
}

void Android_window::on_key_up(Key key)
{
    key_up_signal(move(key));
}

void Android_window::init_app_()
{
    app_->userData = this;
    app_->onAppCmd = on_app_cmd;
    app_->onInputEvent = on_input_event;
}

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Platform