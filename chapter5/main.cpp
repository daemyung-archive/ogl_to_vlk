//
// This file is part of the "ogl_to_vlk" project
// See "LICENSE" for license information.
//

#include <iostream>
#include <string>
#include <sc/Spirv_compiler.h>

using namespace std;
using namespace Sc_lib;

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    const string glsl {
        "precision mediump float;                          \n"
        "                                                  \n"
        "layout(location = 0) in vec2 texcoord;            \n"
        "layout(location = 0) out vec4 color;              \n"
        "                                                  \n"
        "layout(set = 0, binding = 0) uniform sampler2D s; \n"
        "                                                  \n"
        "void main()                                       \n"
        "{                                                 \n"
        "    color = texture(s, texcoord);                 \n"
        "}                                                 \n"
    };

    Spirv_compiler compiler;
    vector<uint32_t> spirv;

    try {
        spirv = compiler.compile(Shader_type::fragment, glsl);
        cout << "compile is done and code length is " << spirv.size() << endl;
    }
    catch (exception& e) {
        cout << "fail to compile\n" << e.what() << endl;
    }

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
