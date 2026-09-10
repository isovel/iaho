#pragma once

#include <utility>

#include <DynamicOutput/DynamicOutput.hpp>
#include <File/Macros.hpp>

#include "Config.hpp"

namespace IAHO
{
    template <int32_t Level = RC::LogLevel::Normal, typename... Args>
    auto Log(RC::File::StringViewType Format, Args&&... FormatArgs) -> void
    {
        RC::Output::send<Level>(RC::File::StringType{STR("[IAlreadyHaveOne] ")} + RC::File::StringType{Format}, std::forward<Args>(FormatArgs)...);
    }

    // Per-call hook tracing. Silent unless config.ini raises LogLevel to Discovery.
    template <typename... Args>
    auto LogDiscovery(RC::File::StringViewType Format, Args&&... FormatArgs) -> void
    {
        if (GetConfig().Verbosity() < LogVerbosity::Discovery)
        {
            return;
        }
        Log<RC::LogLevel::Verbose>(Format, std::forward<Args>(FormatArgs)...);
    }
} // namespace IAHO
