//
// This file is part of the "sc" project
// See "LICENSE" for license information.
//

#ifndef SC_INCLUDER_GUARD
#define SC_INCLUDER_GUARD

#include <vector>
#include <map>
#include <glslang/Public/ShaderLang.h>
#include <ghc/filesystem.hpp>

namespace Sc {

//----------------------------------------------------------------------------------------------------------------------

namespace fs = ghc::filesystem;

//----------------------------------------------------------------------------------------------------------------------

class Includer final : public glslang::TShader::Includer {
public:
    explicit Includer(std::vector<fs::path> pathes);

    IncludeResult* includeLocal(const char* headerName,
                                const char* includerName,
                                size_t inclusionDepth) override;

    IncludeResult* includeSystem(const char* headerName,
                                 const char* includerName,
                                 size_t inclusionDepth) override;

    void releaseInclude(IncludeResult *result) override;

private:
    std::vector<fs::path> pathes_;
    std::map<std::string, std::string> sources_;
};

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Sc

#endif // SC_INCLUDER_GUARD
