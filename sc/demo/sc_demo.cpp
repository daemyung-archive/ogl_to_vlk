//
// This file is part of the "sc" project
// See "LICENSE" for license information.
//

#include <cassert>
#include <fstream>
#include <iostream>
#include <sc/Spirv_compiler.h>
#include <sc/Spirv_reflector.h>
#include <sc/Glsl_compiler.h>
#include <sc/Msl_compiler.h>

using namespace std;
using namespace Sc;

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    vector<uint32_t> spirv_src;

    try {
        fstream fin("../../../sc/demo/sc_demo.glsl");

        assert(fin.is_open());
        auto glsl_src = std::string { istreambuf_iterator<char>(fin), istreambuf_iterator<char>() };

        assert(!glsl_src.empty());
        spirv_src = Spirv_compiler().compile(Shader_type::fragment, glsl_src);
    }
    catch(exception& e) {
        cout << e.what() << endl;
    }

    try {
        auto signature = Spirv_reflector().reflect(spirv_src);

        cout << "buffer size: " << signature.buffers.size() << endl;
        cout << "texture size: " << signature.textures.size() << endl;
        cout << "type size: " << signature.types.size() << endl;
    }
    catch(exception& e) {
        cout << e.what() << endl;
    }

    string glsl_src;

    try {
        glsl_src = Glsl_compiler().compile(spirv_src);
        cout << "glsl :\n" << glsl_src << endl;
    }
    catch(exception& e) {
        cout << e.what() << endl;
    }

    string msl_src;

    try {
        msl_src = Msl_compiler().compile(spirv_src);
        cout << "msl :\n" << msl_src << endl;
    }
    catch(exception& e) {
        cout << e.what() << endl;
    }
}

//----------------------------------------------------------------------------------------------------------------------
