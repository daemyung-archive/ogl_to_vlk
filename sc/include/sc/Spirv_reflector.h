//
// This file is part of the "sc" project
// See "LICENSE" for license information.
//

#ifndef SC_SPIRV_REFLECTOR_GUARD
#define SC_SPIRV_REFLECTOR_GUARD

#include <string>
#include <vector>
#include <unordered_map>
#include "enums.h"

namespace Sc {

//----------------------------------------------------------------------------------------------------------------------

struct Member {
    std::string type;
    std::string name;
    uint32_t array {1u};
    uint32_t offset {UINT32_MAX};
};

//----------------------------------------------------------------------------------------------------------------------

struct Type {
    std::string name;
    std::vector<Member> members;
};

//----------------------------------------------------------------------------------------------------------------------

struct Buffer {
    uint32_t binding {UINT32_MAX};
    std::string type;
    std::string name;
    uint32_t size {0};
};

//----------------------------------------------------------------------------------------------------------------------

struct Image {
    uint32_t binding {UINT32_MAX};
    std::string type;
    std::string name;
    std::string format;
};

//----------------------------------------------------------------------------------------------------------------------

struct Texture {
    uint32_t binding {UINT32_MAX};
    std::string type;
    std::string name;
};

//----------------------------------------------------------------------------------------------------------------------

struct Signature {
    std::unordered_map<uint32_t, Buffer> buffers;
    std::unordered_map<uint32_t, Image> images;
    std::unordered_map<uint32_t, Texture> textures;
    std::unordered_map<std::string, Type> types;
};

//----------------------------------------------------------------------------------------------------------------------

class Spirv_reflector final {
public:
    Signature reflect(const std::vector<uint32_t>& src);
};

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Sc

#endif // SC_SPIRV_REFLECTOR_GUARD
