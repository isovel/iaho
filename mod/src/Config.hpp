#pragma once

#include <memory>

#include <IAHO/Settings.hpp>

#include <Unreal/NameTypes.hpp>

namespace IAHO::Config
{
    // Game thread. Reads config.json beside the mod, importing config.ini once
    // when config.json is absent. Every field keeps its default when absent or
    // unreadable.
    auto Initialize() -> void;

    // Game thread, every frame. Polls the file and puts an edit live.
    auto Tick() -> void;

    auto Shutdown() -> void;

    // Any thread. Defaults until Initialize has run.
    auto Get() -> std::shared_ptr<const Settings>;

    // Item ids the player is never allowed to see re-offered, even if the Essential container says otherwise.
    auto IsForceHidden(const Settings& Current, const RC::Unreal::FName& ProductId) -> bool;
    // Item ids that must always stay craftable regardless of ownership.
    auto IsNeverHidden(const Settings& Current, const RC::Unreal::FName& ProductId) -> bool;
} // namespace IAHO::Config
