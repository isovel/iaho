#include "Hooks.hpp"

#include <array>
#include <unordered_map>

#include <Unreal/Core/Containers/Array.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/NameTypes.hpp>
#include <Unreal/UFunctionStructs.hpp>
#include <Unreal/UObject.hpp>

#include "Config.hpp"
#include "Inventory.hpp"
#include "Log.hpp"
#include "RecipeFilter.hpp"
#include "Reflection.hpp"

namespace IAHO
{
    using namespace RC;
    using namespace RC::Unreal;

    namespace
    {
        struct HookTarget
        {
            const CharType* FunctionPath;
            // Name of the TArray<FName> parameter holding the recipe list the menu ends up showing.
            const CharType* RecipeListParam;
            // Parameter carrying a world context, or nullptr to use the hooked object itself.
            const CharType* WorldContextParam;
        };

        // Every list-producing entry point a crafting surface might use. Which one actually feeds which
        // menu is resolved at runtime; hooking all of them costs nothing when a menu never calls them.
        constexpr std::array Targets{
            HookTarget{STR("/Script/Pal.PalUIUtility:FilteringWorkSpaceRecipe"), STR("OutFilteredArray"), STR("WorldContextObject")},
            HookTarget{STR("/Script/Pal.PalTechnologyData:FilteringUnlockedRecipe"), STR("OutRecipeIdArray"), nullptr},
            HookTarget{STR("/Script/Pal.PalMapObjectConvertItemModel:GetRecipes"), STR("ReturnValue"), nullptr},
        };

        auto LastHiddenCount() -> std::unordered_map<const CharType*, int32>&
        {
            static std::unordered_map<const CharType*, int32> Counts{};
            return Counts;
        }

        auto ParamAddress(UnrealScriptFunctionCallableContext& Context, UFunction* Function, const CharType* ParamName) -> void*
        {
            auto* Property = Function->FindProperty(FName{ParamName, FNAME_Add});
            if (!Property)
            {
                return nullptr;
            }
            auto* Locals = Context.TheStack.Locals();
            if (!Locals)
            {
                return nullptr;
            }
            return Locals + Property->GetOffset_Internal();
        }

        auto DescribeList(const TArray<FName>& List) -> File::StringType
        {
            File::StringType Description{};
            for (int32 Index = 0; Index < List.Num(); ++Index)
            {
                if (Index > 0)
                {
                    Description += STR(", ");
                }
                Description += FName{List[Index]}.ToString();
            }
            return Description;
        }

        auto OnRecipeList(UnrealScriptFunctionCallableContext& Context, void* CustomData) -> void
        {
            const auto* Target = static_cast<const HookTarget*>(CustomData);
            if (!Target || !GetConfig().IsEnabled())
            {
                return;
            }

            auto* Function = Context.TheStack.Node();
            if (!Function)
            {
                return;
            }

            auto* ListAddress = ParamAddress(Context, Function, Target->RecipeListParam);
            if (!ListAddress)
            {
                Log<LogLevel::Error>(STR("'{}' has no parameter '{}'\n"), Target->FunctionPath, Target->RecipeListParam);
                return;
            }
            auto& RecipeList = *static_cast<TArray<FName>*>(ListAddress);

            UObject* WorldContext = Context.Context;
            if (Target->WorldContextParam)
            {
                if (auto* ContextAddress = ParamAddress(Context, Function, Target->WorldContextParam))
                {
                    if (auto* Provided = *static_cast<UObject**>(ContextAddress))
                    {
                        WorldContext = Provided;
                    }
                }
            }
            if (!WorldContext)
            {
                return;
            }

            LogDiscovery(STR("{} produced {} recipe(s): {}\n"), Target->FunctionPath, RecipeList.Num(), DescribeList(RecipeList));

            // Fail open: without a readable key item inventory the vanilla list stands untouched.
            if (!GetEssentialInventory().Refresh(WorldContext))
            {
                return;
            }

            int32 Hidden = 0;
            for (int32 Index = RecipeList.Num() - 1; Index >= 0; --Index)
            {
                const auto RecipeId = RecipeList[Index];
                if (!GetRecipeFilter().ShouldHide(WorldContext, RecipeId))
                {
                    continue;
                }
                LogDiscovery(STR("Hiding '{}' (already in key items)\n"), FName{RecipeId}.ToString());
                RecipeList.RemoveAt(Index);
                ++Hidden;
            }

            // A crafting menu can re-query its list every frame, so only report when the answer changes.
            auto& LastHidden = LastHiddenCount()[Target->FunctionPath];
            if (Hidden != LastHidden)
            {
                LastHidden = Hidden;
                if (Hidden > 0)
                {
                    Log(STR("{}: hid {} already-owned key item recipe(s)\n"), Target->FunctionPath, Hidden);
                }
            }
        }
    } // namespace

    auto InstallHooks() -> void
    {
        for (const auto& Target : Targets)
        {
            auto* Function = FindFunction(Target.FunctionPath);
            if (!Function)
            {
                Log<LogLevel::Warning>(STR("Hook target '{}' not found in this build\n"), Target.FunctionPath);
                continue;
            }
            Function->RegisterPostHook(&OnRecipeList, const_cast<HookTarget*>(&Target));
            Log(STR("Hooked '{}'\n"), Target.FunctionPath);
        }
    }
} // namespace IAHO
