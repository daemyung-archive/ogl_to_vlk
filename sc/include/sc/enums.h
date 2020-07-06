//
// This file is part of the "sc" project
// See "LICENSE" for license information.
//

#ifndef SC_ENUMS_GUARD
#define SC_ENUMS_GUARD

#include <cstdint>

namespace Sc {

//----------------------------------------------------------------------------------------------------------------------

enum class Shader_type : uint8_t {
    vertex = 0, fragment, compute
};

//----------------------------------------------------------------------------------------------------------------------

enum class Platform : uint8_t {
    embeded = 0, desktop
};

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Sc

#endif // SC_ENUMS_GUARD
