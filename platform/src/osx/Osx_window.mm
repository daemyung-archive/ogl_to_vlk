//
// This file is part of the "platform" project
// See "LICENSE" for license information.
//

#include <utility>
#include <Carbon/Carbon.h>
#include "osx/Osx_window.h"

using namespace std;
using namespace Platform;

namespace {

//----------------------------------------------------------------------------------------------------------------------

inline Extent to_extent(NSSize size)
{
    return { static_cast<uint32_t>(size.width), static_cast<uint32_t>(size.height), 1 };
}

//----------------------------------------------------------------------------------------------------------------------

inline NSString* to_string(const wstring& str)
{
    return [[NSString alloc] initWithBytes:&str[0]
                                    length:str.size() * sizeof(wchar_t)
                                  encoding:NSUTF32LittleEndianStringEncoding];
}

inline NSWindowStyleMask to_style_mask(const Osx_window_desc& desc)
{
    auto style_mask = NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;

    if (!desc.title.empty())
        style_mask |= NSWindowStyleMaskTitled;

    if (desc.resizable)
        style_mask |= NSWindowStyleMaskResizable;

    return style_mask;
}

//----------------------------------------------------------------------------------------------------------------------

} // namespace of

//----------------------------------------------------------------------------------------------------------------------

@interface App_delegate : NSObject<NSApplicationDelegate>

- (id)init:(Osx_window*)window;

@end

@implementation App_delegate
{
    Osx_window* window_;
    NSTimer* timer_;
}

//----------------------------------------------------------------------------------------------------------------------

- (id)init:(Osx_window*)window
{
    self = [super init];

    if (self)
        window_ = window;

    return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    window_->on_startup();
    [self start_timer_];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)applicationWillTerminate:(NSNotification*)notification
{
    [self stop_timer_];
    window_->on_shutdown();
}

//----------------------------------------------------------------------------------------------------------------------

- (void)start_timer_
{
    timer_ = [NSTimer scheduledTimerWithTimeInterval:(1.0 / 60.0)
                                              target:self
                                            selector:@selector(on_time_tick_)
                                            userInfo:nil
                                             repeats:YES];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)stop_timer_
{
    [timer_ invalidate];
    timer_ = nil;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)on_time_tick_
{
    window_->on_render();
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface Window_delegate : NSObject<NSWindowDelegate>

- (id)init:(Osx_window*)window;

@end

@implementation Window_delegate
{
    Osx_window* window_;
}

//----------------------------------------------------------------------------------------------------------------------

- (id)init:(Osx_window*)window
{
    self = [super init];

    if (self)
        window_ = window;

    return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize
{
    window_->on_resize(to_extent(frameSize));

    return frameSize;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface View : NSView

- (id)init:(Osx_window*)window;

@end

@implementation View
{
    Osx_window* window_;
}

//----------------------------------------------------------------------------------------------------------------------

- (id)init:(Osx_window*)window
{
    self = [super init];

    if (self)
        window_ = window;

    return self;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isOpaque
{
    return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)canBecomeKeyView
{
    return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)acceptsFirstResponder
{
    return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseDown:(NSEvent *)event
{
    window_->touch_down_signal();
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseDragged:(NSEvent *)event
{
    auto point = [self convertPoint:event.locationInWindow fromView:nil];

    window_->touch_move_signal(point.x, self.bounds.size.height - point.y);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseUp:(NSEvent *)event
{
    window_->touch_up_signal();
}

//----------------------------------------------------------------------------------------------------------------------

- (void)mouseMoved:(NSEvent *)event
{
    auto point = [self convertPoint:event.locationInWindow fromView:nil];

    window_->touch_move_signal(point.x, self.bounds.size.height - point.y);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)keyDown:(NSEvent*)event
{
    switch (event.keyCode) {
        case kVK_ANSI_1: {
            window_->on_key_down(Key::one);
            break;
        }
        case kVK_ANSI_2: {
            window_->on_key_down(Key::two);
            break;
        }
        case kVK_ANSI_3: {
            window_->on_key_down(Key::three);
            break;
        }
        case kVK_ANSI_4: {
            window_->on_key_down(Key::four);
            break;
        }
        case kVK_ANSI_5: {
            window_->on_key_down(Key::five);
            break;
        }
        case kVK_ANSI_6: {
            window_->on_key_down(Key::six);
            break;
        }
        case kVK_ANSI_7: {
            window_->on_key_down(Key::seven);
            break;
        }
        case kVK_ANSI_8: {
            window_->on_key_down(Key::eight);
            break;
        }
        case kVK_ANSI_9: {
            window_->on_key_down(Key::nine);
            break;
        }
        case kVK_ANSI_0: {
            window_->on_key_down(Key::zero);
            break;
        }
        case kVK_ANSI_Q: {
            window_->on_key_down(Key::q);
            break;
        }
        case kVK_ANSI_W: {
            window_->on_key_down(Key::w);
            break;
        }
        case kVK_ANSI_E: {
            window_->on_key_down(Key::e);
            break;
        }
        case kVK_ANSI_R: {
            window_->on_key_down(Key::r);
            break;
        }
        case kVK_ANSI_T: {
            window_->on_key_down(Key::t);
            break;
        }
        case kVK_ANSI_Y: {
            window_->on_key_down(Key::y);
            break;
        }
        case kVK_ANSI_U: {
            window_->on_key_down(Key::u);
            break;
        }
        case kVK_ANSI_I: {
            window_->on_key_down(Key::i);
            break;
        }
        case kVK_ANSI_O: {
            window_->on_key_down(Key::o);
            break;
        }
        case kVK_ANSI_P: {
            window_->on_key_down(Key::p);
            break;
        }
        case kVK_ANSI_A: {
            window_->on_key_down(Key::q);
            break;
        }
        case kVK_ANSI_S: {
            window_->on_key_down(Key::s);
            break;
        }
        case kVK_ANSI_D: {
            window_->on_key_down(Key::d);
            break;
        }
        case kVK_ANSI_F: {
            window_->on_key_down(Key::f);
            break;
        }
        case kVK_ANSI_G: {
            window_->on_key_down(Key::g);
            break;
        }
        case kVK_ANSI_H: {
            window_->on_key_down(Key::h);
            break;
        }
        case kVK_ANSI_J: {
            window_->on_key_down(Key::j);
            break;
        }
        case kVK_ANSI_K: {
            window_->on_key_down(Key::k);
            break;
        }
        case kVK_ANSI_L: {
            window_->on_key_down(Key::l);
            break;
        }
        case kVK_ANSI_Z: {
            window_->on_key_down(Key::z);
            break;
        }
        case kVK_ANSI_X: {
            window_->on_key_down(Key::x);
            break;
        }
        case kVK_ANSI_C: {
            window_->on_key_down(Key::c);
            break;
        }
        case kVK_ANSI_V: {
            window_->on_key_down(Key::v);
            break;
        }
        case kVK_ANSI_B: {
            window_->on_key_down(Key::b);
            break;
        }
        case kVK_ANSI_N: {
            window_->on_key_down(Key::n);
            break;
        }
        case kVK_ANSI_M: {
            window_->on_key_down(Key::m);
            break;
        }
        default: {
            [super keyDown:event];
            break;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------

- (void)keyUp:(NSEvent*)event
{
    switch (event.keyCode) {
        case kVK_ANSI_1: {
            window_->on_key_up(Key::one);
            break;
        }
        case kVK_ANSI_2: {
            window_->on_key_up(Key::two);
            break;
        }
        case kVK_ANSI_3: {
            window_->on_key_up(Key::three);
            break;
        }
        case kVK_ANSI_4: {
            window_->on_key_up(Key::four);
            break;
        }
        case kVK_ANSI_5: {
            window_->on_key_up(Key::five);
            break;
        }
        case kVK_ANSI_6: {
            window_->on_key_up(Key::six);
            break;
        }
        case kVK_ANSI_7: {
            window_->on_key_up(Key::seven);
            break;
        }
        case kVK_ANSI_8: {
            window_->on_key_up(Key::eight);
            break;
        }
        case kVK_ANSI_9: {
            window_->on_key_up(Key::nine);
            break;
        }
        case kVK_ANSI_0: {
            window_->on_key_up(Key::zero);
            break;
        }
        case kVK_ANSI_Q: {
            window_->on_key_up(Key::q);
            break;
        }
        case kVK_ANSI_W: {
            window_->on_key_up(Key::w);
            break;
        }
        case kVK_ANSI_E: {
            window_->on_key_up(Key::e);
            break;
        }
        case kVK_ANSI_R: {
            window_->on_key_up(Key::r);
            break;
        }
        case kVK_ANSI_T: {
            window_->on_key_up(Key::t);
            break;
        }
        case kVK_ANSI_Y: {
            window_->on_key_up(Key::y);
            break;
        }
        case kVK_ANSI_U: {
            window_->on_key_up(Key::u);
            break;
        }
        case kVK_ANSI_I: {
            window_->on_key_up(Key::i);
            break;
        }
        case kVK_ANSI_O: {
            window_->on_key_up(Key::o);
            break;
        }
        case kVK_ANSI_P: {
            window_->on_key_up(Key::p);
            break;
        }
        case kVK_ANSI_A: {
            window_->on_key_up(Key::q);
            break;
        }
        case kVK_ANSI_S: {
            window_->on_key_up(Key::s);
            break;
        }
        case kVK_ANSI_D: {
            window_->on_key_up(Key::d);
            break;
        }
        case kVK_ANSI_F: {
            window_->on_key_up(Key::f);
            break;
        }
        case kVK_ANSI_G: {
            window_->on_key_up(Key::g);
            break;
        }
        case kVK_ANSI_H: {
            window_->on_key_up(Key::h);
            break;
        }
        case kVK_ANSI_J: {
            window_->on_key_up(Key::j);
            break;
        }
        case kVK_ANSI_K: {
            window_->on_key_up(Key::k);
            break;
        }
        case kVK_ANSI_L: {
            window_->on_key_up(Key::l);
            break;
        }
        case kVK_ANSI_Z: {
            window_->on_key_up(Key::z);
            break;
        }
        case kVK_ANSI_X: {
            window_->on_key_up(Key::x);
            break;
        }
        case kVK_ANSI_C: {
            window_->on_key_up(Key::c);
            break;
        }
        case kVK_ANSI_V: {
            window_->on_key_up(Key::v);
            break;
        }
        case kVK_ANSI_B: {
            window_->on_key_up(Key::b);
            break;
        }
        case kVK_ANSI_N: {
            window_->on_key_up(Key::n);
            break;
        }
        case kVK_ANSI_M: {
            window_->on_key_up(Key::m);
            break;
        }
        default: {
            [super keyUp:event];
            break;
        }
    }
}

@end

//----------------------------------------------------------------------------------------------------------------------

namespace Platform {

//----------------------------------------------------------------------------------------------------------------------

Osx_window::Osx_window(const Osx_window_desc& desc) :
    title_ { desc.title },
    extent_ { desc.extent },
    style_mask_ { to_style_mask(desc) },
    window_ { nil }
{
    if (!NSApp)
        init_app_();
}

//----------------------------------------------------------------------------------------------------------------------

void Osx_window::run()
{
    [NSApp run];
}

//----------------------------------------------------------------------------------------------------------------------

void Osx_window::on_startup()
{
    init_window_();
    startup_signal();
}

//----------------------------------------------------------------------------------------------------------------------

void Osx_window::on_shutdown()
{
    shutdown_signal();
}

//----------------------------------------------------------------------------------------------------------------------

void Osx_window::on_render()
{
    render_signal();
}

//----------------------------------------------------------------------------------------------------------------------

void Osx_window::on_resize(const Extent& extent)
{
    extent_ = extent;
    resize_signal(extent_);
}

//----------------------------------------------------------------------------------------------------------------------

void Osx_window::on_key_down(Key key)
{
    key_down_signal(move(key));
}

//----------------------------------------------------------------------------------------------------------------------

void Osx_window::on_key_up(Key key)
{
    key_up_signal(move(key));
}

//----------------------------------------------------------------------------------------------------------------------

void Osx_window::init_app_()
{
    // create the app.
    NSApp = [NSApplication sharedApplication];

    // set up the app.
    assert(NSApp);
    [NSApp setDelegate:[[App_delegate alloc] init:this]];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps: YES];
    [NSApp finishLaunching];
}

//----------------------------------------------------------------------------------------------------------------------

void Osx_window::init_window_()
{
    // create a window.
    auto rect = NSMakeRect(0.0f, 0.0f, extent_.w, extent_.h);

    window_ = [[NSWindow alloc] initWithContentRect:rect
                                          styleMask:style_mask_
                                            backing:NSBackingStoreBuffered
                                              defer:NO];

    // set up a window.
    assert(window_);
    if (!title_.empty())
        [window_ setTitle:to_string(title_)];

    [window_ setDelegate:[[Window_delegate alloc] init:this]];
    [window_ setContentView:[[View alloc] init:this]];
    [window_ setAcceptsMouseMovedEvents:YES];
    [window_ center];
    [window_ makeKeyAndOrderFront:nil];

    // calculate an extent by hdpi.
    uint32_t scale = [window_ backingScaleFactor];

    assert(0 != scale);
    extent_ = extent_ * Extent { scale, scale, 1 };
}

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Platform
