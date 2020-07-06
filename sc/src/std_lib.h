//
// This file is part of the "sc" project
// See "LICENSE" for license information.
//

#ifndef SC_STD_LIB_MODULES_GUARD
#define SC_STD_LIB_MODULES_GUARD

#include <cassert>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <fstream>
#include <array>
#include <map>
#include <utility>

//----------------------------------------------------------------------------------------------------------------------

template<typename T>
inline auto etoi(T e) noexcept
{
    return static_cast<typename std::underlying_type<T>::type>(e);
}

//----------------------------------------------------------------------------------------------------------------------

#endif // SC_STD_LIB_MODULES_GUARD
