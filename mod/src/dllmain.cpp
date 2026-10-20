#include <Mod/CppUserModBase.hpp>
#include <PalCore/Runtime/GameThread.hpp>

#include "Config.hpp"
#include "Hooks.hpp"
#include "Log.hpp"

class IAlreadyHaveOne : public RC::CppUserModBase
{
  public:
    IAlreadyHaveOne() : CppUserModBase()
    {
        ModName = STR("IAlreadyHaveOne");
        ModVersion = STR("0.1.0");
        ModDescription = STR("Hides crafting recipes for key items the player already owns.");
        ModAuthors = STR("isovel");
    }

    ~IAlreadyHaveOne() override
    {
        // The config stays: the recipe hook outlives this object and reads it.
        PalCore::GameThread::Stop();
    }

    auto on_unreal_init() -> void override
    {
        if (!PalCore::GameThread::Start(STR("IAlreadyHaveOne")))
        {
            IAHO::Log::Error(STR("Neither EngineTick nor ProcessEvent could be hooked"));
            return;
        }
        IAHO::Config::Initialize();
        // Registered after Initialize, so the poll never overlaps the first load.
        PalCore::GameThread::Tasks().AddFrameCallback([](float) { IAHO::Config::Tick(); });

        // Installed whatever "enabled" says: the hook checks it on every call,
        // so an edit to config.json takes effect without a restart.
        IAHO::InstallHooks();
    }
};

extern "C"
{
    __declspec(dllexport) RC::CppUserModBase* start_mod()
    {
        return new IAlreadyHaveOne();
    }

    __declspec(dllexport) void uninstall_mod(RC::CppUserModBase* Mod)
    {
        delete Mod;
    }
}
