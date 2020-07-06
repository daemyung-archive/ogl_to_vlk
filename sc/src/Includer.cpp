//
// This file is part of the "sc" project
// See "LICENSE" for license information.
//

#include "std_lib.h"
#include "Includer.h"

using namespace std;

namespace Sc {

//----------------------------------------------------------------------------------------------------------------------

Includer::Includer(std::vector<fs::path> pathes) :
    pathes_ {move(pathes)}
{
}

//----------------------------------------------------------------------------------------------------------------------

Includer::IncludeResult* Includer::includeLocal(const char* headerName,
                                                const char* includerName,
                                                size_t inclusionDepth)
{
    pathes_.emplace_back(includerName);

    for (auto& path : pathes_) {
        ifstream fin(path.string() + headerName, ios::binary);

        if (!fin.is_open())
            continue;

        auto& src = sources_[path] = { istreambuf_iterator<char>(fin),
                                       istreambuf_iterator<char>() };

        return new IncludeResult { path, &src[0], src.size(), nullptr };
    }

    return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

Includer::IncludeResult* Includer::includeSystem(const char* headerName,
                                                 const char* includerName,
                                                 size_t inclusionDepth)
{
    return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

void Includer::releaseInclude(IncludeResult *result)
{
    if (result)
        sources_.erase(result->headerName);
}

//----------------------------------------------------------------------------------------------------------------------

} // of namespace Sc
