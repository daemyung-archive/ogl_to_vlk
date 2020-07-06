//
// This file is part of the "platform" project
// See "LICENSE" for license information.
//

#ifndef PLATFORM_WINDOWS_LIBRARY_GUARD
#define PLATFORM_WINDOWS_LIBRARY_GUARD

#include <string>
#include <Windows.h>
#include <ghc/filesystem.hpp>

namespace Platform {

//----------------------------------------------------------------------------------------------------------------------

namespace fs = ghc::filesystem;

//----------------------------------------------------------------------------------------------------------------------

class Windows_library final {
public:
    Windows_library();

    Windows_library(const fs::path& path);

    Windows_library(Windows_library&& other) noexcept;

    ~Windows_library();

    Windows_library& operator=(Windows_library&& other) noexcept;

    template<typename T>
    T symbol(const char* name) const
    {
        return reinterpret_cast<T>(GetProcAddress(module_, name));
    }

private:
    void init_module_(const fs::path& path);

    void term_module_();

private:
    HMODULE module_;
};

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Platform

#endif // PLATFORM_WINDOWS_LIBRARY_GUARD
