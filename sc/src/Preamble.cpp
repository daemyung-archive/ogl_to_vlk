//
// This file is part of the "sc" project
// See "LICENSE" for license information.
//

#include <fmt/format.h>
#include "std_lib.h"
#include "Preamble.h"

using namespace std;
using namespace fmt;

namespace {

//----------------------------------------------------------------------------------------------------------------------

inline auto is_valid(const std::string& token)
{
    constexpr array<const char*, 2> directives = {
        "#define", "#undef"
    };

    for (auto& directive : directives) {
        if (string::npos != token.find(directive))
            return true;
    }

    return false;
}

//----------------------------------------------------------------------------------------------------------------------

} // of namespace

namespace Sc {

//----------------------------------------------------------------------------------------------------------------------

Preamble::Preamble() noexcept :
    tokens_ {}
{
}

void Preamble::token(const std::string& token)
{
    if (!is_valid(token))
        throw format("{} is invalid.", token);

    tokens_.append(token + '\n');
}

//----------------------------------------------------------------------------------------------------------------------

std::string Preamble::str() const noexcept
{
    return tokens_;
}

//----------------------------------------------------------------------------------------------------------------------

bool Preamble::empty() const noexcept
{
    return tokens_.empty();
}

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Sc
