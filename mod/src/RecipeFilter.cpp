#include "RecipeFilter.hpp"

#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/UObjectGlobals.hpp>

#include "Config.hpp"
#include "Inventory.hpp"
#include "Log.hpp"
#include <PalCore/Runtime/Reflect.hpp>

namespace IAHO
{
    using namespace RC;
    using namespace RC::Unreal;

    namespace
    {
        // Byte offset of FPalItemRecipe::Product_Id, read from reflection rather than assumed.
        auto ProductIdOffsetInRecipe() -> int32
        {
            static int32 Offset = [] {
                auto* RecipeStruct = UObjectGlobals::StaticFindObject<UScriptStruct*>(nullptr, nullptr, STR("/Script/Pal.PalItemRecipe"));
                if (!RecipeStruct)
                {
                    Log::Error(STR("FPalItemRecipe struct not found"));
                    return -1;
                }
                auto* Property = RecipeStruct->FindProperty(FName{STR("Product_Id"), FNAME_Add});
                if (!Property)
                {
                    Log::Error(STR("FPalItemRecipe has no Product_Id property"));
                    return -1;
                }
                return static_cast<int32>(Property->GetOffset_Internal());
            }();
            return Offset;
        }
    } // namespace

    auto RecipeFilter::FindRecipeAccess(UObject* WorldContext) -> UObject*
    {
        auto* Utility = PalCore::DefaultObject(STR("/Script/Pal.PalMasterDataTablesUtility"));
        if (!Utility)
        {
            return nullptr;
        }

        PalCore::Call GetAccess(Utility, STR("GetItemRecipeDataTableAccess"));
        if (!GetAccess.Set(STR("WorldContextObject"), WorldContext) || !GetAccess.Run())
        {
            return nullptr;
        }
        UObject** Access = GetAccess.Get<UObject*>(STR("ReturnValue"));
        return Access ? *Access : nullptr;
    }

    auto RecipeFilter::ResolveProductId(UObject* WorldContext, const FName& RecipeId) -> FName
    {
        const auto Key = RecipeId.ToUnstableInt();
        if (const auto Cached = m_ProductByRecipe.find(Key); Cached != m_ProductByRecipe.end())
        {
            return Cached->second;
        }

        FName Product{};
        const auto Offset = ProductIdOffsetInRecipe();
        auto* Access = FindRecipeAccess(WorldContext);

        if (Access && Offset >= 0)
        {
            PalCore::Call FindRow(Access, STR("BP_FindRow"));
            if (FindRow.Set(STR("RowName"), RecipeId) && FindRow.Run())
            {
                const bool* Found = FindRow.Get<bool>(STR("bResult"));
                auto* Row = Found && *Found ? static_cast<uint8_t*>(FindRow.Raw(STR("ReturnValue"))) : nullptr;
                if (Row)
                {
                    Product = *reinterpret_cast<FName*>(Row + Offset);
                }
            }
        }

        // Palworld's recipe rows are keyed by their product item id, so the row name is the natural fallback
        // when the master data table is not reachable yet (for example very early in a level load).
        if (!Product.ToUnstableInt())
        {
            Product = RecipeId;
            Log::Debug(STR("Recipe '{}' product unresolved, assuming row name is the product id"), FName{RecipeId}.ToString());
        }

        m_ProductByRecipe.emplace(Key, Product);
        return Product;
    }

    auto RecipeFilter::ShouldHide(const Settings& Current, UObject* WorldContext, const FName& RecipeId) -> bool
    {
        const auto Product = ResolveProductId(WorldContext, RecipeId);

        if (Config::IsNeverHidden(Current, Product) || Config::IsNeverHidden(Current, RecipeId))
        {
            return false;
        }
        if (Config::IsForceHidden(Current, Product) || Config::IsForceHidden(Current, RecipeId))
        {
            return true;
        }
        return GetEssentialInventory().Owns(Product);
    }

    auto GetRecipeFilter() -> RecipeFilter&
    {
        static RecipeFilter Instance{};
        return Instance;
    }
} // namespace IAHO
