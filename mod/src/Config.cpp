#include "Config.hpp"

#include <chrono>
#include <optional>
#include <string>

#include <IAHO/LegacyIni.hpp>
#include <PalCfg/ConfigFile.hpp>
#include <PalCfg/File.hpp>
#include <PalCfg/LogSink.hpp>
#include <PalCfg/Utf.hpp>
#include <PalCore/Runtime/Module.hpp>

#include "Log.hpp"

namespace IAHO::Config
{
    namespace
    {
        using File = PalCfg::ConfigFile<Settings, kFields.size()>;

        // Function-local, so none of it is constructed inside DllMain.
        struct State
        {
            std::optional<File> ConfigFile;
            PalCfg::CallbackSink Sink{[](PalCfg::Severity Severity, const std::string& Line) {
                const std::wstring Text = PalCfg::Utf8ToWide(Line);
                switch (Severity)
                {
                case PalCfg::Severity::Error: Log::Error(STR("config: {}"), Text); break;
                case PalCfg::Severity::Warning: Log::Warn(STR("config: {}"), Text); break;
                default: Log::Info(STR("config: {}"), Text); break;
                }
            }};
            // Holds an import whose config.json could not be written, so it
            // still runs this session.
            std::shared_ptr<const Settings> Override;
        };

        auto GetState() -> State&
        {
            static State Instance;
            return Instance;
        }

        auto Publish(const Settings& Current) -> void
        {
            Log::SetVerbosity(Current.Verbosity);
            Log::Info(STR("Config: Enabled={} LogLevel={} ExtraHidden={} NeverHide={}"),
                      Current.Enabled,
                      static_cast<int>(Current.Verbosity),
                      Current.ExtraHiddenRecipes.size(),
                      Current.NeverHideRecipes.size());
        }

        // config.ini is read here and nowhere else, and only while config.json
        // does not exist yet.
        auto ImportLegacy(File& ConfigFile) -> bool
        {
            if (PalCfg::StatFile(ConfigFile.Path()).exists)
            {
                return false;
            }
            const std::optional<std::string> IniPath = PalCore::ModuleRelative(reinterpret_cast<const void*>(&Initialize), "../config.ini");
            std::string Text;
            if (!IniPath || !PalCfg::ReadFileText(*IniPath, Text))
            {
                return false;
            }

            const Settings Imported = ImportIni(Text);
            if (ConfigFile.Save(Imported))
            {
                Log::Info(STR("imported config.ini into config.json"));
            }
            else
            {
                Log::Warn(STR("could not write config.json; running on config.ini's settings this session"));
                GetState().Override = std::make_shared<const Settings>(Imported);
            }
            return true;
        }
    } // namespace

    auto Initialize() -> void
    {
        auto& S = GetState();
        const std::optional<std::string> Path = PalCore::ModuleRelative(reinterpret_cast<const void*>(&Initialize), "../config.json");
        if (!Path)
        {
            Log::Error(STR("could not resolve the mod folder, using the default config"));
            return;
        }

        S.ConfigFile.emplace(kFields, *Path, kSchema.ModId());
        S.ConfigFile->SetSink(S.Sink);
        S.ConfigFile->SetOnReload([](const Settings& Current) {
            GetState().Override.reset();
            Publish(Current);
        });

        if (!ImportLegacy(*S.ConfigFile))
        {
            S.ConfigFile->LoadOrCreate();
        }
        Publish(*Get());
    }

    auto Tick() -> void
    {
        if (auto& S = GetState(); S.ConfigFile)
        {
            S.ConfigFile->Tick(std::chrono::steady_clock::now());
        }
    }

    auto Shutdown() -> void
    {
        GetState().ConfigFile.reset();
        GetState().Override.reset();
    }

    auto Get() -> std::shared_ptr<const Settings>
    {
        auto& S = GetState();
        if (S.Override)
        {
            return S.Override;
        }
        if (S.ConfigFile)
        {
            return S.ConfigFile->Get();
        }
        static const auto Defaults = std::make_shared<const Settings>();
        return Defaults;
    }

    auto IsForceHidden(const Settings& Current, const RC::Unreal::FName& ProductId) -> bool
    {
        return !Current.ExtraHiddenRecipes.empty() && Current.ExtraHiddenRecipes.contains(RC::Unreal::FName{ProductId}.ToString());
    }

    auto IsNeverHidden(const Settings& Current, const RC::Unreal::FName& ProductId) -> bool
    {
        return !Current.NeverHideRecipes.empty() && Current.NeverHideRecipes.contains(RC::Unreal::FName{ProductId}.ToString());
    }
} // namespace IAHO::Config
