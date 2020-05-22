//
// This file is part of the "ogl_to_vlk" project
// See "LICENSE" for license information.
//

#include <iostream>
#include <string>
#include <sc/Spirv_compiler.h>

using namespace std;
using namespace Sc;

//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    // 프레그먼트 셰이더를 정의한다. 셰이더에서 사용된 문법은 VKSL이다.
    const string vksl {
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

    // VKSL을 SPIR-V로 변환하기 위한 런타임 컴파일러를 생성한다.
    Spirv_compiler compiler;

    // VKSL을 SPIR-V로 변경하며 실패하면 예외가 발생한다.
    vector<uint32_t> spirv;
    try {
        spirv = compiler.compile(Shader_type::fragment, vksl);
        cout << "compile is done and code length is " << spirv.size() << endl;
    }
    catch (exception& e) {
        cout << "fail to compile\n" << e.what() << endl;
    }

    // 실제 어플리케이션을 출시할 때는 런타임 컴파일러를 사용하지 않는다. 왜냐하면 런타임 컴파일러가 생각보다 무겁기 때문이다.
    // 그러므로 런타임 컴파일러는 개발용으로만 사용하고 출시에는 미리 컴파일된 SPIR-V를 사용한다.

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
