//
// This file is part of the "sc" project
// See "LICENSE" for license information.
//

#ifndef SC_MSL_COMPILER_GUARD
#define SC_MSL_COMPILER_GUARD

#include <string>
#include <vector>
#include "enums.h"

namespace Sc {

//----------------------------------------------------------------------------------------------------------------------

struct Msl_compiler_configs final {
    Platform platform;
};

//----------------------------------------------------------------------------------------------------------------------

class Msl_compiler final {
public:
    Msl_compiler() noexcept;

    explicit Msl_compiler(const Msl_compiler_configs& configs) noexcept;

    std::string compile(const std::vector<uint32_t>& src);

private:
    Msl_compiler_configs configs_;
};

//----------------------------------------------------------------------------------------------------------------------

}  // of namespace Sc

#endif // SC_MSL_COMPILER_GUARD
