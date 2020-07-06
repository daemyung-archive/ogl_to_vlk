//
// This file is part of the "platform" project
// See "LICENSE" for license information.
//

#include <cassert>
#include <stdexcept>
#include "windows/Windows_library.h"

using namespace std;

namespace {

//----------------------------------------------------------------------------------------------------------------------

std::wstring convert(const std::string& utf8)
{
    auto length = MultiByteToWideChar(CP_UTF8, 0,
                                      &utf8[0], static_cast<int>(utf8.length()),
                                      nullptr, 0);

    assert(length > 0);
    auto utf16 = std::wstring(length, u'\0');

    MultiByteToWideChar(CP_UTF8, 0,
                        &utf8[0], static_cast<int>(utf8.length()),
                        &utf16[0], static_cast<int>(utf16.length()));

    return utf16;
};

//----------------------------------------------------------------------------------------------------------------------

}

namespace Platform {

//----------------------------------------------------------------------------------------------------------------------

Windows_library::Windows_library() :
    module_ { NULL }
{
}

//----------------------------------------------------------------------------------------------------------------------

Windows_library::Windows_library(const fs::path& path) :
    module_ { NULL }
{
    init_module_(path);
}

//----------------------------------------------------------------------------------------------------------------------

Windows_library::Windows_library(Windows_library&& other) noexcept :
    module_ { other.module_ }
{
    other.module_ = NULL;
}

//----------------------------------------------------------------------------------------------------------------------

Windows_library::~Windows_library()
{
    term_module_();
}

//----------------------------------------------------------------------------------------------------------------------

Windows_library& Windows_library::operator=(Windows_library&& other) noexcept
{
    swap(module_, other.module_);

    return *this;
}

//----------------------------------------------------------------------------------------------------------------------

void Windows_library::init_module_(const fs::path& path)
{
    module_ = LoadLibrary(convert(path.string()).c_str());

    if (!module_)
        throw runtime_error("fail to create a library");
}

//----------------------------------------------------------------------------------------------------------------------

void Windows_library::term_module_()
{
    if (module_)
        FreeLibrary(module_);
}

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Platform
