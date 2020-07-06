//
// This file is part of the "sc" project
// See "LICENSE" for license information.
//

#include <spirv_cross/spirv_glsl.hpp>
#include <platform/build_target.h>
#include "std_lib.h"
#include "Glsl_compiler.h"

using namespace spirv_cross;

namespace Sc {

//----------------------------------------------------------------------------------------------------------------------

Glsl_compiler::Glsl_compiler() noexcept :
#if TARGET_OS_IOS || defined(__ANDROID__)
    configs_ {Platform::embeded}
#elif TARGET_OS_OSX || defined(_WIN32)
    configs_ {Platform::desktop}
#endif
{
}

//----------------------------------------------------------------------------------------------------------------------

Glsl_compiler::Glsl_compiler(const Glsl_compiler_configs& configs) noexcept :
    configs_ {configs}
{
}

//----------------------------------------------------------------------------------------------------------------------

std::string Glsl_compiler::compile(const std::vector<uint32_t>& src)
{
    CompilerGLSL compiler(src);

    // set up the common option.
    auto common_options = compiler.get_common_options();

    switch (configs_.platform) {
        case Platform::embeded:
            common_options.es = true;
            common_options.version = 310;
            break;
        case Platform::desktop:
            common_options.version = 450;
            break;
    }

    common_options.vertex.fixup_clipspace = true;

    compiler.set_common_options(common_options);

    // compile to GLSL from SPIRV.
    auto output = compiler.compile();

    assert(!output.empty());
    return output;
}

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Sc
