//
// This file is part of the "platform" project
// See "LICENSE" for license information.
//

#ifndef PLATFORM_EXTENT_GUARD
#define PLATFORM_EXTENT_GUARD

#include <cstdint>
#include <algorithm>

namespace Platform {

//----------------------------------------------------------------------------------------------------------------------

struct Extent {
    uint32_t w { 0 };
    uint32_t h { 0 };
    uint32_t d { 0 };
};

//----------------------------------------------------------------------------------------------------------------------

inline Extent operator*(const Extent& lhs, const Extent& rhs)
{
    return { lhs.w * rhs.w, lhs.h * rhs.h, lhs.d * rhs.d };
}

//----------------------------------------------------------------------------------------------------------------------

inline bool operator==(const Extent&lhs, const Extent& rhs)
{
    return (lhs.w == rhs.w) && (lhs.h == rhs.h) && (lhs.d == rhs.d);
}

//----------------------------------------------------------------------------------------------------------------------

inline bool operator!=(const Extent& lhs, const Extent& rhs)
{
    return !(lhs == rhs);
}

//----------------------------------------------------------------------------------------------------------------------

inline Extent min(const Extent& lhs, const Extent& rhs)
{
    return { std::min(lhs.w, rhs.w), std::min(lhs.h, rhs.h), std::min(lhs.d, rhs.d) };
}

//----------------------------------------------------------------------------------------------------------------------

inline Extent max(const Extent& lhs, const Extent& rhs)
{
    return { std::max(lhs.w, rhs.w), std::max(lhs.h, rhs.h), std::max(lhs.d, rhs.d) };
}

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Platform

#endif // PLATFORM_EXTENT_GUARD
