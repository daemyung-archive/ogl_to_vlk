//
// This file is part of the "sc" project
// See "LICENSE" for license information.
//

#include <SPIRV/GlslangToSpv.h>
#include <spirv-tools/optimizer.hpp>
#include <fmt/format.h>
#include "std_lib.h"
#include "Spirv_compiler.h"
#include "Includer.h"

using namespace std;
using namespace glslang;
using namespace spv;
using namespace spvtools;
using namespace fmt;
using namespace Sc;

namespace {

//----------------------------------------------------------------------------------------------------------------------

constexpr auto client_semantic_version = 100;
constexpr TLimits default_limits {
    /* .nonInductiveForLoops = */ true,
    /* .whileLoops = */ true,
    /* .doWhileLoops = */ true,
    /* .generalUniformIndexing = */ true,
    /* .generalAttributeMatrixVectorIndexing = */ true,
    /* .generalVaryingIndexing = */ true,
    /* .generalSamplerIndexing = */ true,
    /* .generalVariableIndexing = */ true,
    /* .generalConstantMatrixVectorIndexing = */ true,
};
constexpr TBuiltInResource default_resource = {
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,
    /* .maxDualSourceDrawBuffersEXT = */ 1,
    /* .limits = */ default_limits
};
constexpr EShLanguage languages[] = {EShLangVertex, EShLangFragment, EShLangCompute};

//----------------------------------------------------------------------------------------------------------------------

inline auto to_shader_type(const std::string& extension)
{
    if (".vert" == extension)
        return Shader_type::vertex;

    if (".frag" == extension)
        return Shader_type::fragment;

    if (".comp" == extension)
        return Shader_type::compute;

    throw runtime_error("fail to convert an extension");
}

//----------------------------------------------------------------------------------------------------------------------

} // of namespace

namespace Sc {

//----------------------------------------------------------------------------------------------------------------------

Spirv_compiler::Spirv_compiler() noexcept :
#if TARGET_OS_IOS
    configs_ {Platform::embeded},
#elif TARGET_OS_OSX
    configs_ {Platform::desktop},
#elif defined(__ANDROID__)
    configs_ {Platform::embeded},
#endif
    include_paths_ {}
{
    glslang::InitializeProcess();
}

//----------------------------------------------------------------------------------------------------------------------

Spirv_compiler::Spirv_compiler(const Spirv_compiler_configs &configs) noexcept :
    configs_ {configs},
    include_paths_ {}
{
    glslang::InitializeProcess();
}

//----------------------------------------------------------------------------------------------------------------------

Spirv_compiler::~Spirv_compiler()
{
    glslang::FinalizeProcess();
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<uint32_t> Spirv_compiler::compile(Shader_type type, const std::string &src)
{
    assert(!src.empty());

    // add version and extensions.
    stringstream sin;

    sin << "#extension GL_GOOGLE_include_directive : enable" << '\n'
        << src;

    // compile to spirv from vksl.
    auto output = compile_(type, sin.str());

    // optimize spirv.
    if (configs_.optimize)
        optimize_(output);

    return output;
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<uint32_t> Spirv_compiler::compile(const fs::path& path)
{
    return compile(to_shader_type(path.extension()), read_(path));
}

//----------------------------------------------------------------------------------------------------------------------

void Spirv_compiler::add_include_path(const fs::path& path) noexcept
{
    include_paths_.push_back(path);
}

//----------------------------------------------------------------------------------------------------------------------

std::string Spirv_compiler::read_(const fs::path& path) const
{
    // open a file.
    ifstream fin(path, ios::binary);

    if (!fin.is_open())
        throw runtime_error("fail to open " + path.string());

    // read source from a file.
    std::string src {istreambuf_iterator<char>(fin), istreambuf_iterator<char>()};

    if (src.empty())
        throw runtime_error(path.string() + " is empty");

    return src;
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<uint32_t> Spirv_compiler::compile_(Shader_type type, const std::string& src) const
{
    auto language = languages[etoi(type)];

    // create a shader.
    TShader shader(language);

    // set up shader environment.
    shader.setEnvInput(EShSourceGlsl, language, EShClientVulkan, client_semantic_version);
    shader.setEnvClient(EShClientVulkan, EShTargetVulkan_1_1);
    shader.setEnvTarget(EShTargetSpv, EShTargetSpv_1_0);

    // set up preamble.
    string tokens = preamble_.str();

    if (!tokens.empty())
        shader.setPreamble(&tokens[0]);

    // set up source.
    const auto src_ptr = &src[0];

    assert(src_ptr);
    shader.setStrings(&src_ptr, 1);

    // configure include paths.
    Includer includer {include_paths_};

    // parse shader.
    if (!shader.parse(&default_resource,
                      310, EEsProfile,
                      true, false,
                      EShMsgDefault,
                      includer)) {
        throw runtime_error(format("fail to parse shader\n info : {}\n info debug : {}",
                                   shader.getInfoLog(), shader.getInfoDebugLog()));
    }

    // create a program.
    TProgram program;

    // configure program.
    program.addShader(&shader);

    // link program.
    if (!program.link(EShMsgDefault)) {
        throw runtime_error(format("fail to link program\n info : {}\n info debug : {}",
                                   program.getInfoLog(), program.getInfoDebugLog()));
    }

    // map IO program.
    if (!program.mapIO()) {
        throw runtime_error(format("fail to map IO\n info : {}\n info debug : {}",
                                   program.getInfoLog(), program.getInfoDebugLog()));
    }

    // set up spv options.
    SpvOptions options;

    options.disableOptimizer = !configs_.optimize;
    options.optimizeSize = configs_.optimize;
    options.validate = configs_.validate;

    // compile to SPIRV from intermediate.
    vector<uint32_t> output;

    GlslangToSpv(*program.getIntermediate(language), output, nullptr, &options);

    assert(!output.empty());
    return output;
}

//----------------------------------------------------------------------------------------------------------------------

void Spirv_compiler::optimize_(std::vector<uint32_t>& source)
{
    Optimizer optimizer {SPV_ENV_VULKAN_1_1};

    optimizer.RegisterPass(CreateEliminateDeadFunctionsPass());
    optimizer.RegisterPass(CreateEliminateDeadConstantPass());
    optimizer.RegisterPass(CreateDeadBranchElimPass());

    if (!optimizer.Run(&source[0], source.size(), &source))
        throw runtime_error("fail to optimize shader");
}

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Sc
