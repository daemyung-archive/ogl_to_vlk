//
// This file is part of the "platform" project
// See "LICENSE" for license information.
//

#ifndef PLATFORM_POSIX_LIBRARY_GUARD
#define PLATFORM_POSIX_LIBRARY_GUARD

#include <string>
#include <dlfcn.h>
#include <ghc/filesystem.hpp>

namespace Platform {

//----------------------------------------------------------------------------------------------------------------------

namespace fs = ghc::filesystem;

//----------------------------------------------------------------------------------------------------------------------

class Posix_library final {
public:
	Posix_library();

    Posix_library(const fs::path& path);

    ~Posix_library();

    template<typename T>
    T symbol(const char* name) const
    { return reinterpret_cast<T>(dlsym(library_, name)); }

private:
    void init_library_(const fs::path& path);

    void deinit_library_();

private:
    void* library_;
};

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Platform

#endif // PLATFORM_POSIX_LIBRARY_GUARD
