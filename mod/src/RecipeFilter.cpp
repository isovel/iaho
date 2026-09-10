#include "RecipeFilter.hpp"

#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/UObjectGlobals.hpp>

#include "Config.hpp"
#include "Inventory.hpp"
#include "Log.hpp"
#include "Reflection.hpp"

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
                    Log<LogLevel::Error>(STR("FPalItemRecipe struct not found\n"));
                    return -1;
                }
                auto* Property = RecipeStruct->FindProperty(FName{STR("Product_Id"), FNAME_Add});
                if (!Property)
                {
                    Log<LogLevel::Error>(STR("FPalItemRecipe has no Product_Id property\n"));
                    return -1;
                }
                return static_cast<int32>(Property->GetOffset_Internal());
            }();
            return Offset;
        }
    } // namespace

    auto RecipeFilter::FindRecipeAccess(UObject* WorldContext) -> UObject*
    {
        auto* Utility = FindClassDefaultObject(STR("/Script/Pal.PalMasterDataTablesUtility"));
        if (!Utility)
        {
            return nullptr;
        }

        UObject* Access{};
        FuncCall Call{};
        if (!Call.Bind(Utility, STR("GetItemRecipeDataTableAccess")) || !Call.Set(STR("WorldContextObject"), WorldContext) || !Call.Call() ||
            !Call.Get(STR("ReturnValue"), Access))
        {
            return nullptr;
        }
        return Access;
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
            FuncCall Call{};
            bool Found{};
            if (Call.Bind(Access, STR("BP_FindRow")) && Call.Set(STR("RowName"), RecipeId) && Call.Call() &&
                Call.Get(STR("bResult"), Found) && Found)
            {
                if (auto* Row = static_cast<uint8_t*>(Call.Raw(STR("ReturnValue"))))
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
            LogDiscovery(STR("Recipe '{}' product unresolved, assuming row name is the product id\n"), FName{RecipeId}.ToString());
        }

        m_ProductByRecipe.emplace(Key, Product);
        return Product;
    }

    auto RecipeFilter::ShouldHide(UObject* WorldContext, const FName& RecipeId) -> bool
    {
        const auto Product = ResolveProductId(WorldContext, RecipeId);

        if (GetConfig().IsNeverHidden(Product) || GetConfig().IsNeverHidden(RecipeId))
        {
            return false;
        }
        if (GetConfig().IsForceHidden(Product) || GetConfig().IsForceHidden(RecipeId))
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
