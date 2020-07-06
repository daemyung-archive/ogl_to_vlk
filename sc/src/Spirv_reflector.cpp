//
// This file is part of the "sc" project
// See "LICENSE" for license information.
//

#include <spirv_cross/spirv_reflect.hpp>
#include <rapidjson/document.h>
#include "std_lib.h"
#include "Spirv_reflector.h"

using namespace std;
using namespace spirv_cross;
using namespace rapidjson;
using namespace Sc;

namespace {

//----------------------------------------------------------------------------------------------------------------------

auto to_Members(Value& values)
{
    vector<Member> members {values.Size()};

    for (auto i = 0; i != values.Size(); ++i) {
        auto& member = members[i];
        auto& value = values[i];

        member.type = value["type"].GetString();
        member.name = value["name"].GetString();

        if (value.HasMember("array"))
            member.array = value["array"][0].GetInt();

        member.offset = value["offset"].GetUint();
    }

    return members;
}

//----------------------------------------------------------------------------------------------------------------------

} // of namespace

namespace Sc {

//----------------------------------------------------------------------------------------------------------------------

Signature Spirv_reflector::reflect(const std::vector<uint32_t>& src)
{
    // reflect spirv.
    auto manifest = CompilerReflection(src).compile();

    // parse manifest to document.
    Document document;

    document.Parse(&manifest[0]);

    if (document.HasParseError())
        throw runtime_error("fail to reflect spirv");

    // convert to signature from document.
    Signature signature;

    if (document.HasMember("ubos")) {
        auto& ubos = document["ubos"];

        for (auto i = 0; i != ubos.Size(); ++i) {
            auto binding = ubos[i]["binding"].GetUint();

            Buffer buffer {
                binding,
                ubos[i]["type"].GetString(),
                ubos[i]["name"].GetString(),
                ubos[i]["block_size"].GetUint()
            };

            signature.buffers[binding] = buffer;
        }
    }

    if (document.HasMember("ssbos")) {
        auto& ssbos = document["ssbos"];

        for (auto i = 0; i != ssbos.Size(); ++i) {
            auto binding = ssbos[i]["binding"].GetUint();

            Buffer buffer {
                binding,
                ssbos[i]["type"].GetString(),
                ssbos[i]["name"].GetString(),
                ssbos[i]["block_size"].GetUint()
            };

            signature.buffers[binding] = buffer;
        }
    }

    if (document.HasMember("images")) {
        auto& images = document["images"];

        for (auto i = 0; i != images.Size(); ++i) {
            auto binding = images[i]["binding"].GetUint();

            Image image {
                binding,
                images[i]["type"].GetString(),
                images[i]["name"].GetString(),
                images[i]["format"].GetString(),
            };

            signature.images[binding] = image;
        }
    }

    if (document.HasMember("textures")) {
        auto& textures = document["textures"];

        for (auto i = 0; i != textures.Size(); ++i) {
            auto binding = textures[i]["binding"].GetUint();

            Texture texture {
                binding,
                textures[i]["type"].GetString(),
                textures[i]["name"].GetString(),
            };

            signature.textures[binding] = texture;
        }
    }

    if (document.HasMember("types")) {
        auto& types = document["types"];

        for (auto iter = types.MemberBegin(); iter != types.MemberEnd(); ++iter) {
            auto& name = iter->value["name"];

            if ("gl_PerVertex" == name)
                continue;

            Type type {
                name.GetString(),
                to_Members(iter->value["members"])
            };

            signature.types[iter->name.GetString()] = type;
        }
    }

    return signature;
}

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Sc
