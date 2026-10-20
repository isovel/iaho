#include "Hooks.hpp"

#include <array>
#include <unordered_map>

#include <Unreal/Core/Containers/Array.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
#include <Unreal/NameTypes.hpp>
#include <Unreal/UFunctionStructs.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>

#include "Config.hpp"
#include "Inventory.hpp"
#include "Log.hpp"
#include "RecipeFilter.hpp"

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

        // This is the list every crafting surface actually reads. UPalUIUtility::FilteringWorkSpaceRecipe
        // and UPalTechnologyData::FilteringUnlockedRecipe were also hooked during bring-up: the former
        // returned an empty array on all eight calls, the latter never fired at all.
        constexpr std::array Targets{
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

            // A return value lives in the caller's storage, which RESULT_DECL points at. For a function
            // with no arguments the local frame is not a parameter block at all, so reading a return
            // value off Locals() yields garbage.
            if (Property->HasAllPropertyFlags(CPF_ReturnParm))
            {
                return Context.RESULT_DECL;
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
            // One snapshot per call, so a reload mid-list cannot mix two configs.
            const auto Current = Config::Get();
            if (!Target)
            {
                return;
            }
            if (!Current->Enabled)
            {
                // Forget the last count, so turning the mod back on reports again.
                LastHiddenCount().erase(Target->FunctionPath);
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
                Log::Error(STR("'{}' has no parameter '{}'"), Target->FunctionPath, Target->RecipeListParam);
                return;
            }
            auto& RecipeList = *static_cast<TArray<FName>*>(ListAddress);

            // A misread address shows up as a nonsense length. Refuse to touch the array rather than
            // walk whatever it points at.
            if (RecipeList.Num() < 0 || RecipeList.Num() > 4096)
            {
                Log::Error(STR("'{}' parameter '{}' has implausible length {}, skipping"),
                                     Target->FunctionPath,
                                     Target->RecipeListParam,
                                     RecipeList.Num());
                return;
            }

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

            Log::Debug(STR("{} produced {} recipe(s): {}"), Target->FunctionPath, RecipeList.Num(), DescribeList(RecipeList));

            // Fail open: without a readable key item inventory the vanilla list stands untouched.
            if (!GetEssentialInventory().Refresh(WorldContext))
            {
                return;
            }

            int32 Hidden = 0;
            for (int32 Index = RecipeList.Num() - 1; Index >= 0; --Index)
            {
                const auto RecipeId = RecipeList[Index];
                if (!GetRecipeFilter().ShouldHide(*Current, WorldContext, RecipeId))
                {
                    continue;
                }
                Log::Debug(STR("Hiding '{}' (already in key items)"), FName{RecipeId}.ToString());
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
                    Log::Info(STR("{}: hid {} already-owned key item recipe(s)"), Target->FunctionPath, Hidden);
                }
            }
        }
    } // namespace

    auto InstallHooks() -> void
    {
        // pal-core pins the module, so a Ctrl+R reload runs this same copy with
        // the hook still registered. Unregistering it from the destructor would
        // race the game thread walking the callback list.
        static bool bInstalled = false;
        if (bInstalled)
        {
            return;
        }
        bInstalled = true;

        for (const auto& Target : Targets)
        {
            auto* Function = UObjectGlobals::StaticFindObject<UFunction*>(nullptr, nullptr, Target.FunctionPath);
            if (!Function)
            {
                Log::Warn(STR("Hook target '{}' not found in this build"), Target.FunctionPath);
                continue;
            }
            Function->RegisterPostHook(&OnRecipeList, const_cast<HookTarget*>(&Target));
            Log::Info(STR("Hooked '{}'"), Target.FunctionPath);
        }
    }
} // namespace IAHO
