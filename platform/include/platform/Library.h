//
// This file is part of the "platform" project
// See "LICENSE" for license information.
//

#ifndef PlatformRARY_GUARD
#define PlatformRARY_GUARD

#include "build_target.h"

#if TARGET_OS_OSX || defined(__ANDROID__)
#include "posix/Posix_library.h"
#elif defined(_WIN32)
#include "windows/Windows_library.h"
#endif

namespace Platform {

//----------------------------------------------------------------------------------------------------------------------

#if TARGET_OS_OSX || defined(__ANDROID__)
using Library = Posix_library;
#elif defined(_WIN32)
using Library = Windows_library;
#endif

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Platform

#endif // PlatformRARY_GUARD
