#pragma once

#include <chrono>
#include <unordered_set>

#include <Unreal/NameTypes.hpp>
#include <Unreal/UObject.hpp>

namespace IAHO
{
    /**
     * The set of static item ids currently sitting in the local player's Essential ("key item") container.
     * Palworld keeps key items in their own container, which is exactly the "do I already have one" question.
     */
    class EssentialInventory
    {
      public:
        // Rebuilds from the live container when the cached snapshot is older than the refresh window.
        auto Refresh(RC::Unreal::UObject* WorldContext) -> bool;

        auto Owns(const RC::Unreal::FName& StaticItemId) const -> bool
        {
            return m_OwnedIds.contains(StaticItemId.ToUnstableInt());
        }

        auto Count() const -> size_t
        {
            return m_OwnedIds.size();
        }

        auto Invalidate() -> void
        {
            m_LastRefresh = {};
        }

      private:
        auto ResolveContainer(RC::Unreal::UObject* WorldContext) -> RC::Unreal::UObject*;

        std::unordered_set<uint64_t> m_OwnedIds{};
        std::chrono::steady_clock::time_point m_LastRefresh{};
    };

    auto GetEssentialInventory() -> EssentialInventory&;
} // namespace IAHO
