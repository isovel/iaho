#include "Inventory.hpp"

#include <Unreal/CoreUObject/UObject/UnrealType.hpp>

#include "Config.hpp"
#include "Log.hpp"
#include <PalCore/Runtime/Reflect.hpp>

namespace IAHO
{
    using namespace RC;
    using namespace RC::Unreal;

    namespace
    {
        // EPalPlayerInventoryType::Essential
        constexpr uint8_t EssentialInventoryType = 2;

        // EPalItemTypeB::Essential - the generic key item bucket. Measured in game, this is where the
        // consumables live (KeySphere_01-06, WhaleWhistleFragment_04) while every craft-once item has a
        // dedicated bucket instead: Essential_UnlockPlayerFuture, Essential_PalGear,
        // Essential_AdditionalInventory, Essential_Lamp, Blueprint. An unrecognised item landing here
        // simply keeps its recipe, which is the harmless direction to be wrong in.
        constexpr uint8_t ConsumableKeyItemTypeB = 54;
        constexpr auto RefreshWindow = std::chrono::milliseconds{250};
    } // namespace

    auto EssentialInventory::ResolveContainer(UObject* WorldContext) -> UObject*
    {
        auto* PalUtility = PalCore::DefaultObject(STR("/Script/Pal.PalUtility"));
        if (!PalUtility)
        {
            Log::Error(STR("PalUtility class default object not found"));
            return nullptr;
        }

        PalCore::Call GetPlayerState(PalUtility, STR("GetLocalPlayerState"));
        UObject** PlayerState{};
        if (!GetPlayerState.Set(STR("WorldContextObject"), WorldContext) || !GetPlayerState.Run() ||
            !(PlayerState = GetPlayerState.Get<UObject*>(STR("ReturnValue"))) || !*PlayerState)
        {
            // Expected on a dedicated server, where there is no local player. The mod stays inert there.
            Log::Debug(STR("No local player state; skipping"));
            return nullptr;
        }

        PalCore::Call GetInventoryData(*PlayerState, STR("GetInventoryData"));
        UObject** InventoryData{};
        if (!GetInventoryData.Run() || !(InventoryData = GetInventoryData.Get<UObject*>(STR("ReturnValue"))) || !*InventoryData)
        {
            Log::Debug(STR("No inventory data on local player state"));
            return nullptr;
        }

        PalCore::Call GetContainer(*InventoryData, STR("TryGetContainerFromInventoryType"));
        const bool* Found{};
        UObject** Container{};
        if (!GetContainer.Set(STR("inventoryType"), EssentialInventoryType) || !GetContainer.Run() ||
            !(Found = GetContainer.Get<bool>(STR("ReturnValue"))) || !*Found ||
            !(Container = GetContainer.Get<UObject*>(STR("OutContainer"))))
        {
            Log::Debug(STR("Essential container unavailable"));
            return nullptr;
        }
        return *Container;
    }

    auto EssentialInventory::Refresh(UObject* WorldContext) -> bool
    {
        const auto Now = std::chrono::steady_clock::now();
        if (m_LastRefresh != std::chrono::steady_clock::time_point{} && Now - m_LastRefresh < RefreshWindow)
        {
            return true;
        }

        auto* Container = ResolveContainer(WorldContext);
        if (!Container)
        {
            return false;
        }

        PalCore::Call Num(Container, STR("Num"));
        const int32* SlotCount{};
        if (!Num.Run() || !(SlotCount = Num.Get<int32>(STR("ReturnValue"))))
        {
            Log::Error(STR("Essential container has no reflected Num()"));
            return false;
        }

        if (!PalCore::Call(Container, STR("Get")).Bound())
        {
            Log::Error(STR("Essential container has no reflected Get(Index)"));
            return false;
        }

        std::unordered_set<uint64_t> Owned{};
        for (int32 Index = 0; Index < *SlotCount; ++Index)
        {
            PalCore::Call GetSlot(Container, STR("Get"));
            UObject** SlotAt{};
            if (!GetSlot.Set(STR("Index"), Index) || !GetSlot.Run() || !(SlotAt = GetSlot.Get<UObject*>(STR("ReturnValue"))) || !*SlotAt)
            {
                continue;
            }
            UObject* const Slot = *SlotAt;

            PalCore::Call Empty(Slot, STR("IsEmpty"));
            if (Empty.Run())
            {
                if (const bool* IsEmpty = Empty.Get<bool>(STR("ReturnValue")); IsEmpty && *IsEmpty)
                {
                    continue;
                }
            }

            PalCore::Call GetStatic(Slot, STR("TryGetStaticItemData"));
            const bool* Resolved{};
            UObject** StaticItemDataAt{};
            if (!GetStatic.Run() || !(Resolved = GetStatic.Get<bool>(STR("ReturnValue"))) || !*Resolved ||
                !(StaticItemDataAt = GetStatic.Get<UObject*>(STR("OutStaticItemData"))) || !*StaticItemDataAt)
            {
                continue;
            }
            UObject* const StaticItemData = *StaticItemDataAt;

            FName ItemId{};
            if (!PalCore::ReadProperty(StaticItemData, STR("ID"), ItemId))
            {
                continue;
            }

            uint8_t TypeB{};
            const auto HasTypeB = PalCore::ReadProperty(StaticItemData, STR("TypeB"), TypeB);
            const auto IsConsumable = HasTypeB && TypeB == ConsumableKeyItemTypeB;

            if (!IsConsumable)
            {
                Owned.insert(ItemId.ToUnstableInt());
            }

            if (Log::Get().Enabled(PalCore::Level::Debug))
            {
                int32 MaxStackCount{};
                uint8_t TypeA{};
                bool NotConsumed{};
                PalCore::ReadProperty(StaticItemData, STR("MaxStackCount"), MaxStackCount);
                PalCore::ReadProperty(StaticItemData, STR("TypeA"), TypeA);
                PalCore::ReadProperty(StaticItemData, STR("bNotConsumed"), NotConsumed);
                Log::Debug(STR("Essential slot {} holds '{}' (TypeA={} TypeB={} MaxStack={} NotConsumed={}){}"),
                             Index,
                             ItemId.ToString(),
                             TypeA,
                             TypeB,
                             MaxStackCount,
                             NotConsumed,
                             IsConsumable ? STR(" consumable, stays craftable") : STR(""));
            }
        }

        const auto Changed = Owned != m_OwnedIds;
        m_OwnedIds = std::move(Owned);
        m_LastRefresh = Now;

        if (Changed)
        {
            Log::Info(STR("Key item inventory: {} distinct item(s)"), m_OwnedIds.size());
        }
        return true;
    }

    auto GetEssentialInventory() -> EssentialInventory&
    {
        static EssentialInventory Instance{};
        return Instance;
    }
} // namespace IAHO
