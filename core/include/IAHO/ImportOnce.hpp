#pragma once

#include <string>

#include <IAHO/Settings.hpp>
#include <PalCfg/ConfigFile.hpp>

namespace IAHO
{
    using ConfigFile = PalCfg::ConfigFile<Settings, kFields.size()>;

    struct ImportResult
    {
        bool Imported = false;
        // False when config.json could not be written; Values must then stay
        // live for the session by other means.
        bool Saved = false;
        Settings Values;
    };

    // Carries config.ini into config.json, and only while config.json does not
    // exist yet. config.ini itself is never written.
    ImportResult ImportOnce(ConfigFile& File, const std::string& IniPath);
}
