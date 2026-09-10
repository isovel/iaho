#pragma once

namespace IAHO
{
    // Installs post-hooks on the recipe-list functions that feed Palworld's crafting menus.
    // Safe to call once the Unreal module is initialized; missing functions are reported and skipped.
    auto InstallHooks() -> void;
} // namespace IAHO
