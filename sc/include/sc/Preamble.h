//
// This file is part of the "sc" project
// See "LICENSE" for license information.
//

#ifndef SC_PREAMBLE_GUARD
#define SC_PREAMBLE_GUARD

#include <string>
#include <unordered_set>

namespace Sc {

//----------------------------------------------------------------------------------------------------------------------

class Preamble final {
public:
    Preamble() noexcept;

    void token(const std::string& token);

    [[nodiscard]]
    std::string str() const noexcept;

    [[nodiscard]]
    bool empty() const noexcept;

private:
    std::string tokens_;
};

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Sc

#endif // SC_PREAMBLE_GUARD
