//
// This file is part of the "platform" project
// See "LICENSE" for license information.
//

#include "posix/Posix_library.h"

namespace Platform {

//----------------------------------------------------------------------------------------------------------------------

Posix_library::Posix_library() :
    library_ { nullptr }
{
}

//----------------------------------------------------------------------------------------------------------------------

Posix_library::Posix_library(const fs::path& path) :
    library_ { nullptr }
{
    init_library_(path);
}


//----------------------------------------------------------------------------------------------------------------------

Posix_library::~Posix_library()
{
    deinit_library_();
}

//----------------------------------------------------------------------------------------------------------------------

void Posix_library::init_library_(const fs::path& path)
{
    library_ = dlopen(path.c_str(), RTLD_NOW);
}

//----------------------------------------------------------------------------------------------------------------------

void Posix_library::deinit_library_()
{
    if (library_)
        dlclose(library_);
}

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Platform
