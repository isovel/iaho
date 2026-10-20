#pragma once

#include <format>
#include <string>
#include <utility>

#include <IAHO/Settings.hpp>
#include <PalCore/Log.hpp>
#include <PalCore/Runtime/Log.hpp>
#include <PalCore/Utf.hpp>

namespace IAHO::Log
{
    // Constructed on first use, from on_unreal_init, never inside DllMain.
    inline auto Get() -> PalCore::Logger&
    {
        static PalCore::Logger Instance{"IAlreadyHaveOne"};
        static const bool bSinked = (Instance.SetSink(PalCore::OutputSink()), true);
        (void)bSinked;
        return Instance;
    }

    // Warnings and errors always pass; discovery adds the per-call tracing.
    inline auto SetVerbosity(LogVerbosity Verbosity) -> void
    {
        switch (Verbosity)
        {
        case LogVerbosity::Quiet: Get().SetLevel(PalCore::Level::Warning); break;
        case LogVerbosity::Normal: Get().SetLevel(PalCore::Level::Info); break;
        case LogVerbosity::Discovery: Get().SetLevel(PalCore::Level::Debug); break;
        }
    }

    template <typename... Args>
    auto Write(PalCore::Level At, std::wformat_string<Args...> Format, Args&&... FormatArgs) -> void
    {
        if (!Get().Enabled(At))
        {
            return;
        }
        Get().Write(At, PalCore::Narrow(std::format(Format, std::forward<Args>(FormatArgs)...)));
    }

    template <typename... Args>
    auto Error(std::wformat_string<Args...> Format, Args&&... FormatArgs) -> void
    {
        Write(PalCore::Level::Error, Format, std::forward<Args>(FormatArgs)...);
    }

    template <typename... Args>
    auto Warn(std::wformat_string<Args...> Format, Args&&... FormatArgs) -> void
    {
        Write(PalCore::Level::Warning, Format, std::forward<Args>(FormatArgs)...);
    }

    template <typename... Args>
    auto Info(std::wformat_string<Args...> Format, Args&&... FormatArgs) -> void
    {
        Write(PalCore::Level::Info, Format, std::forward<Args>(FormatArgs)...);
    }

    // Per-call hook tracing, shown only at the discovery log level.
    template <typename... Args>
    auto Debug(std::wformat_string<Args...> Format, Args&&... FormatArgs) -> void
    {
        Write(PalCore::Level::Debug, Format, std::forward<Args>(FormatArgs)...);
    }
}
