#pragma once

#include <filesystem>
#include <unordered_set>

#include <File/Macros.hpp>
#include <Unreal/NameTypes.hpp>

namespace IAHO
{
    enum class LogVerbosity
    {
        Quiet = 0,   // errors only
        Normal = 1,  // lifecycle plus each recipe actually hidden
        Discovery = 2, // full per-call hook tracing, used to resolve which hook feeds each menu
    };

    class Config
    {
      public:
        // Reads config.ini next to the mod dll. Every field keeps its default when absent or unparsable.
        auto Load(const std::filesystem::path& ModDirectory) -> void;

        auto IsEnabled() const -> bool
        {
            return m_Enabled;
        }
        auto Verbosity() const -> LogVerbosity
        {
            return m_Verbosity;
        }

        // Item ids the player is never allowed to see re-offered, even if the Essential container says otherwise.
        auto IsForceHidden(const RC::Unreal::FName& ProductId) const -> bool;
        // Item ids that must always stay craftable regardless of ownership.
        auto IsNeverHidden(const RC::Unreal::FName& ProductId) const -> bool;

      private:
        auto ParseIdList(const RC::File::StringType& Value, std::unordered_set<RC::File::StringType>& Out) -> void;

        bool m_Enabled{true};
        LogVerbosity m_Verbosity{LogVerbosity::Normal};
        std::unordered_set<RC::File::StringType> m_ExtraHidden{};
        std::unordered_set<RC::File::StringType> m_NeverHidden{};
    };

    auto GetConfig() -> Config&;
} // namespace IAHO
