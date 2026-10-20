#pragma once

#include <string_view>

#include <IAHO/Settings.hpp>

namespace IAHO
{
    // Reads the config.ini that config.json replaced, with the old reader's
    // rules, so a user's settings carry across once. UTF-8 in; anything absent
    // or unreadable keeps its default.
    Settings ImportIni(std::string_view Text);
}
