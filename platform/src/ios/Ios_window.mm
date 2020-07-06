//
// This file is part of the "platform" project
// See "LICENSE" for license information.
//

#include "platform/ios/Ios_window.h"

using namespace std;
using namespace Platform;

//----------------------------------------------------------------------------------------------------------------------

static Ios_window* window { nullptr };

//----------------------------------------------------------------------------------------------------------------------

@interface App_delegate : UIResponder<UIApplicationDelegate>
@end

@implementation App_delegate
{
    NSTimer* timer_;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
    window->on_startup();
    [self start_timer_];

    return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)applicationWillTerminate:(UIApplication*)application
{
    [self stop_timer_];
    window->on_shutdown();
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
    window->on_render();
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface View : UIView
@end

@implementation View
{
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isOpaque
{
    return YES;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)isHidden
{
    return NO;
}

//----------------------------------------------------------------------------------------------------------------------

- (BOOL)acceptsFirstResponder
{
    return YES;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@interface View_controller : UIViewController
@end

@implementation View_controller
{
}

//----------------------------------------------------------------------------------------------------------------------

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self.view addSubview:[View new]];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    window->touch_down_signal();
}

//----------------------------------------------------------------------------------------------------------------------

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    auto touch = [touches anyObject];
    auto point = [touch locationInView:nil];

    window->touch_move_signal(point.x, point.y);
}

//----------------------------------------------------------------------------------------------------------------------

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    window->touch_up_signal();
}

@end

//----------------------------------------------------------------------------------------------------------------------

namespace Platform {

//----------------------------------------------------------------------------------------------------------------------

Ios_window::Ios_window(const Ios_window_desc& desc) :
    extent_ { desc.extent },
    window_ { nil }
{
    ::window = this;
}

//----------------------------------------------------------------------------------------------------------------------

void Ios_window::run()
{
    int argc { 0 };
    char* argv[] { nullptr };

    UIApplicationMain(argc, argv, nil, NSStringFromClass([App_delegate class]));
}

//----------------------------------------------------------------------------------------------------------------------

void Ios_window::on_startup()
{
    auto bounds = [[UIScreen mainScreen] bounds];

    extent_.w = bounds.size.width;
    extent_.h = bounds.size.height;

    window_ = [[UIWindow alloc] initWithFrame:bounds];

    auto view_controller = [View_controller new];

    assert(view_controller);
    window_.rootViewController = view_controller;

    [window_ makeKeyAndVisible];
    startup_signal();
}

//----------------------------------------------------------------------------------------------------------------------

void Ios_window::on_shutdown()
{
    shutdown_signal();
}

//----------------------------------------------------------------------------------------------------------------------

void Ios_window::on_render()
{
    render_signal();
}

//----------------------------------------------------------------------------------------------------------------------

void Ios_window::on_resize(const Extent& extent)
{
}

//----------------------------------------------------------------------------------------------------------------------

void Ios_window::on_key_down(Key key)
{
    key_down_signal(move(key));
}

//----------------------------------------------------------------------------------------------------------------------

void Ios_window::on_key_up(Key key)
{
    key_up_signal(move(key));
}

//----------------------------------------------------------------------------------------------------------------------

} // namespace of Platform
