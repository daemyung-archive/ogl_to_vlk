//
// This file is part of the "platform" project
// See "LICENSE" for license information.
//

#ifndef PLATFORM_WINDOW_GUARD
#define PLATFORM_WINDOW_GUARD

#include "build_target.h"

#if TARGET_OS_IOS
#include "ios/Ios_window.h"
#elif TARGET_OS_OSX
#include "osx/Osx_window.h"
#elif defined(__ANDROID__)
#include "android/Android_window.h"
#elif defined(_WIN32)
#include "windows/Windows_window.h"
#endif

namespace Platform {

//----------------------------------------------------------------------------------------------------------------------

#if TARGET_OS_IOS
using Window_desc = Ios_window_desc;
using Window = Ios_window;
#elif TARGET_OS_OSX
using Window_desc = Osx_window_desc;
using Window = Osx_window;
#elif defined(__ANDROID__)
using Window_desc = Android_window_desc;
using Window = Android_window;
#elif  defined(_WIN32)
using Window_desc = Windows_window_desc;
using Window = Windows_window;
#endif

//----------------------------------------------------------------------------------------------------------------------

}

#endif // PLATFORM_WINDOW_GUARD
