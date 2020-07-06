//
// This file is part of the "platform" project
// See "LICENSE" for license information.
//

#include <doctest.h>
#include <platform/Window.h>

using namespace std;
using namespace doctest;
using namespace Platform;

namespace {

//----------------------------------------------------------------------------------------------------------------------

const wstring default_title = L"window test suite";
const Extent default_extent = {256, 256, 1};

//----------------------------------------------------------------------------------------------------------------------

} // namespace of

TEST_SUITE_BEGIN("window test suite");

//----------------------------------------------------------------------------------------------------------------------

TEST_CASE("create a window with a valid description")
{
    Window_desc desc;

    desc.title = default_title;
    desc.extent = default_extent;

    auto window = make_unique<Window>(desc);
    CHECK(window);

    REQUIRE(window->title() == default_title);
    REQUIRE(window->extent() == default_extent);
}

//----------------------------------------------------------------------------------------------------------------------

TEST_SUITE_END();
