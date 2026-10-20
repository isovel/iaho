#pragma once

#include <array>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

#include <PalCfg/Schema.hpp>

namespace IAHO
{
    enum class LogVerbosity
    {
        Quiet,     // errors and warnings only
        Normal,    // lifecycle plus each recipe actually hidden
        Discovery, // full per-call hook tracing, used to resolve which hook feeds each menu
    };

    struct Settings
    {
        bool Enabled = true;
        LogVerbosity Verbosity = LogVerbosity::Normal;
        std::unordered_set<std::wstring> ExtraHiddenRecipes;
        std::unordered_set<std::wstring> NeverHideRecipes;
    };
}

template <>
struct PalCfg::EnumNames<IAHO::LogVerbosity>
{
    static constexpr std::array<std::pair<std::string_view, IAHO::LogVerbosity>, 3> kValues{{
        {"quiet", IAHO::LogVerbosity::Quiet},
        {"normal", IAHO::LogVerbosity::Normal},
        {"discovery", IAHO::LogVerbosity::Discovery},
    }};
};

namespace IAHO
{
    // config.ini matched keys in any case, so config.json does too.
    inline constexpr auto kSchema = PalCfg::Schema<Settings>("IAlreadyHaveOne")
        .CaseInsensitiveKeys()
        .Field("enabled", &Settings::Enabled)
            .Label("Enabled")
            .Help("Master switch. Set to false to restore vanilla crafting menus without\nremoving the mod.")
        .Field("logLevel", &Settings::Verbosity)
            .Label("Log level")
            .Help("quiet     - errors and warnings only\n"
                  "normal    - lifecycle plus one line per recipe hidden\n"
                  "discovery - full per-call tracing of the recipe list and every hide decision,\n"
                  "            then set it back to normal")
        .Field("extraHiddenRecipes", &Settings::ExtraHiddenRecipes)
            .Label("Extra hidden recipes")
            .Help("Item ids to hide once owned even if the key item rule would not catch them.")
        .Field("neverHideRecipes", &Settings::NeverHideRecipes)
            .Label("Never hide recipes")
            .Help("Item ids that must always stay craftable.");

    inline constexpr auto kFields = kSchema.Flatten();
}
