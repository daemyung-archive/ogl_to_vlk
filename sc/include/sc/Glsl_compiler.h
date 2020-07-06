//
// This file is part of the "sc" project
// See "LICENSE" for license information.
//

#ifndef SC_GLSL_COMPILER_GUARD
#define SC_GLSL_COMPILER_GUARD

#include <string>
#include <vector>
#include "enums.h"

namespace Sc {

//----------------------------------------------------------------------------------------------------------------------

struct Glsl_compiler_configs final {
    Platform platform;
};

//----------------------------------------------------------------------------------------------------------------------

class Glsl_compiler final {
public:
    Glsl_compiler() noexcept;

    explicit Glsl_compiler(const Glsl_compiler_configs& configs) noexcept;

    std::string compile(const std::vector<uint32_t>& src);

private:
    Glsl_compiler_configs configs_;
};

//----------------------------------------------------------------------------------------------------------------------

}  // of namespace Sc

#endif // SC_GLSL_COMPILER_GUARD
