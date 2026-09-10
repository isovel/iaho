#include <filesystem>

#include <Windows.h>

#include <Mod/CppUserModBase.hpp>

#include "Config.hpp"
#include "Hooks.hpp"
#include "Log.hpp"

namespace
{
    HMODULE GSelfModule{};

    // The dll lives in <ModDirectory>/dlls/main.dll, so config.ini sits two levels up.
    auto ModDirectory() -> std::filesystem::path
    {
        wchar_t Path[MAX_PATH]{};
        if (GSelfModule && GetModuleFileNameW(GSelfModule, Path, MAX_PATH) > 0)
        {
            return std::filesystem::path{Path}.parent_path().parent_path();
        }
        return std::filesystem::current_path();
    }
} // namespace

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

    ~IAlreadyHaveOne() override = default;

    auto on_unreal_init() -> void override
    {
        IAHO::GetConfig().Load(ModDirectory());
        if (!IAHO::GetConfig().IsEnabled())
        {
            IAHO::Log(STR("Disabled via config.ini\n"));
            return;
        }
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

BOOL APIENTRY DllMain(HMODULE Module, DWORD Reason, LPVOID)
{
    if (Reason == DLL_PROCESS_ATTACH)
    {
        GSelfModule = Module;
        DisableThreadLibraryCalls(Module);
    }
    return TRUE;
}
