#include "Config.hpp"

#include <atomic>
#include <chrono>
#include <optional>
#include <string>

#include <IAHO/ImportOnce.hpp>
#include <PalCfg/File.hpp>
#include <PalCfg/LogSink.hpp>
#include <PalCfg/Utf.hpp>
#include <PalCore/Runtime/Module.hpp>

#include "Log.hpp"

namespace IAHO::Config
{
    namespace
    {
        // Function-local, so none of it is constructed inside DllMain. pal-core
        // pins the module, so this outlives every Ctrl+R reload: the recipe hook
        // stays registered across one and may read it from the game thread at
        // any moment, so it is built once and never torn down.
        struct State
        {
            std::optional<ConfigFile> File;
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
            std::atomic<std::shared_ptr<const Settings>> Override;
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
    } // namespace

    auto Initialize() -> void
    {
        auto& S = GetState();
        if (S.File)
        {
            // A Ctrl+R reload: the file and its poll carry on as they were.
            Publish(*Get());
            return;
        }

        const std::optional<std::string> Path = PalCore::ModuleRelative(reinterpret_cast<const void*>(&Initialize), "../config.json");
        if (!Path)
        {
            Log::Error(STR("could not resolve the mod folder, using the default config"));
            return;
        }

        S.File.emplace(kFields, *Path, kSchema.ModId());
        S.File->SetSink(S.Sink);
        S.File->SetOnReload([](const Settings& Current) {
            GetState().Override.store(nullptr);
            Publish(Current);
        });

        const std::optional<std::string> IniPath = PalCore::ModuleRelative(reinterpret_cast<const void*>(&Initialize), "../config.ini");
        const ImportResult Import = IniPath ? ImportOnce(*S.File, *IniPath) : ImportResult{};
        if (!Import.Imported)
        {
            S.File->LoadOrCreate();
        }
        else if (Import.Saved)
        {
            Log::Info(STR("imported config.ini into config.json"));
        }
        else
        {
            Log::Warn(STR("could not write config.json; running on config.ini's settings this session"));
            S.Override.store(std::make_shared<const Settings>(Import.Values));
        }
        Publish(*Get());
    }

    auto Tick() -> void
    {
        if (auto& S = GetState(); S.File)
        {
            S.File->Tick(std::chrono::steady_clock::now());
        }
    }

    auto Get() -> std::shared_ptr<const Settings>
    {
        auto& S = GetState();
        if (auto Override = S.Override.load())
        {
            return Override;
        }
        if (S.File)
        {
            return S.File->Get();
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
