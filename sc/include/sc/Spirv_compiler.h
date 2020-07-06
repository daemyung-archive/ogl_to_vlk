//
// This file is part of the "sc" project
// See "LICENSE" for license information.
//

#ifndef SC_SPIRV_COMPILER_GUARD
#define SC_SPIRV_COMPILER_GUARD

#include <vector>
#include <ghc/filesystem.hpp>
#include "enums.h"
#include "Preamble.h"

namespace Sc {

//----------------------------------------------------------------------------------------------------------------------

namespace fs = ghc::filesystem;

//----------------------------------------------------------------------------------------------------------------------

struct Spirv_compiler_configs final {
    Platform platform;
    bool optimize {true};
    bool validate {false};
};

//----------------------------------------------------------------------------------------------------------------------

class Spirv_compiler final {
public:
    Spirv_compiler() noexcept;

    explicit Spirv_compiler(const Spirv_compiler_configs &configs) noexcept;

    ~Spirv_compiler();

    std::vector<uint32_t> compile(Shader_type type, const std::string& src);

    std::vector<uint32_t> compile(const fs::path& path);

    void add_include_path(const fs::path& path) noexcept;

    inline void preamble(const Preamble& preamble) noexcept
    { preamble_ = preamble; }

    inline void configs(const Spirv_compiler_configs& configs) noexcept
    { configs_ = configs; }

private:
    [[nodiscard]]
    std::string read_(const fs::path& path) const;

    [[nodiscard]]
    std::vector<uint32_t> compile_(Shader_type type, const std::string& src) const;

    void optimize_(std::vector<uint32_t>& source);

private:
    Spirv_compiler_configs configs_;
    Preamble preamble_;
    std::vector<fs::path> include_paths_;
};

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Sc

#endif // SC_SPIRV_COMPILER_GUARD
