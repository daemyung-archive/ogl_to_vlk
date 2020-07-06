//
// This file is part of the "sc" project
// See "LICENSE" for license information.
//

#include <spirv_cross/spirv_msl.hpp>
#include <platform/build_target.h>
#include "std_lib.h"
#include "Msl_compiler.h"

using namespace std;
using namespace spv;
using namespace spirv_cross;

namespace {

//----------------------------------------------------------------------------------------------------------------------

constexpr CompilerMSL::Options::Platform platforms[] { CompilerMSL::Options::iOS, CompilerMSL::Options::macOS };

//----------------------------------------------------------------------------------------------------------------------

} // of namespace

namespace Sc {

//----------------------------------------------------------------------------------------------------------------------

Msl_compiler::Msl_compiler() noexcept :
#if TARGET_OS_IOS || defined(__ANDROID__)
    configs_ {Platform::embeded}
#elif TARGET_OS_OSX || defined(_WIN32)
    configs_ {Platform::desktop}
#endif
{
}

//----------------------------------------------------------------------------------------------------------------------

Msl_compiler::Msl_compiler(const Msl_compiler_configs& configs) noexcept :
    configs_ {configs}
{
}

//----------------------------------------------------------------------------------------------------------------------

std::string Msl_compiler::compile(const std::vector<uint32_t>& src)
{
    CompilerMSL compiler(src);

    // set up the common options.
    auto common_options = compiler.get_common_options();

    common_options.es = true;
    common_options.version = 310;

    compiler.set_common_options(common_options);

    // set up the msl options.
    auto msl_options = compiler.get_msl_options();

    msl_options.platform = platforms[etoi(configs_.platform)];
    msl_options.enable_decoration_binding = true;

    compiler.set_msl_options(msl_options);

    // compile to MSL from SPIRV.
    auto output = compiler.compile();

    assert(!output.empty());
    return output;
}

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Sc
