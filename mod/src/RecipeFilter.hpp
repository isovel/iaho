#pragma once

#include <unordered_map>

#include <Unreal/NameTypes.hpp>
#include <Unreal/UObject.hpp>

namespace IAHO
{
    /**
     * Answers "should this crafting recipe be hidden right now" by resolving the recipe's product item
     * and asking the Essential inventory whether the player already holds one.
     */
    class RecipeFilter
    {
      public:
        // Product item id a recipe row yields. NAME_None when the row cannot be resolved.
        auto ResolveProductId(RC::Unreal::UObject* WorldContext, const RC::Unreal::FName& RecipeId) -> RC::Unreal::FName;

        auto ShouldHide(RC::Unreal::UObject* WorldContext, const RC::Unreal::FName& RecipeId) -> bool;

        auto ClearCache() -> void
        {
            m_ProductByRecipe.clear();
        }

      private:
        auto FindRecipeAccess(RC::Unreal::UObject* WorldContext) -> RC::Unreal::UObject*;

        std::unordered_map<uint64_t, RC::Unreal::FName> m_ProductByRecipe{};
    };

    auto GetRecipeFilter() -> RecipeFilter&;
} // namespace IAHO
