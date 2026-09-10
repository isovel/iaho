#include "Inventory.hpp"

#include <Unreal/CoreUObject/UObject/UnrealType.hpp>

#include "Config.hpp"
#include "Log.hpp"
#include "Reflection.hpp"

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

        template <typename ValueType>
        auto ReadProperty(UObject* Object, const CharType* PropertyName, ValueType& OutValue) -> bool
        {
            if (!Object)
            {
                return false;
            }
            auto* Property = Object->GetPropertyByNameInChain(PropertyName);
            if (!Property)
            {
                return false;
            }
            OutValue = *Property->ContainerPtrToValuePtr<ValueType>(Object);
            return true;
        }
    } // namespace

    auto EssentialInventory::ResolveContainer(UObject* WorldContext) -> UObject*
    {
        auto* PalUtility = FindClassDefaultObject(STR("/Script/Pal.PalUtility"));
        if (!PalUtility)
        {
            Log<LogLevel::Error>(STR("PalUtility class default object not found\n"));
            return nullptr;
        }

        UObject* PlayerState{};
        FuncCall GetPlayerState{};
        if (!GetPlayerState.Bind(PalUtility, STR("GetLocalPlayerState")) || !GetPlayerState.Set(STR("WorldContextObject"), WorldContext) ||
            !GetPlayerState.Call() || !GetPlayerState.Get(STR("ReturnValue"), PlayerState) || !PlayerState)
        {
            // Expected on a dedicated server, where there is no local player. The mod stays inert there.
            LogDiscovery(STR("No local player state; skipping\n"));
            return nullptr;
        }

        UObject* InventoryData{};
        FuncCall GetInventoryData{};
        if (!GetInventoryData.Bind(PlayerState, STR("GetInventoryData")) || !GetInventoryData.Call() ||
            !GetInventoryData.Get(STR("ReturnValue"), InventoryData) || !InventoryData)
        {
            LogDiscovery(STR("No inventory data on local player state\n"));
            return nullptr;
        }

        UObject* Container{};
        bool Found{};
        FuncCall GetContainer{};
        if (!GetContainer.Bind(InventoryData, STR("TryGetContainerFromInventoryType")) ||
            !GetContainer.Set(STR("inventoryType"), EssentialInventoryType) || !GetContainer.Call() ||
            !GetContainer.Get(STR("ReturnValue"), Found) || !Found || !GetContainer.Get(STR("OutContainer"), Container))
        {
            LogDiscovery(STR("Essential container unavailable\n"));
            return nullptr;
        }
        return Container;
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

        int32 SlotCount{};
        FuncCall Num{};
        if (!Num.Bind(Container, STR("Num")) || !Num.Call() || !Num.Get(STR("ReturnValue"), SlotCount))
        {
            Log<LogLevel::Error>(STR("Essential container has no reflected Num()\n"));
            return false;
        }

        FuncCall GetSlot{};
        if (!GetSlot.Bind(Container, STR("Get")))
        {
            Log<LogLevel::Error>(STR("Essential container has no reflected Get(Index)\n"));
            return false;
        }

        std::unordered_set<uint64_t> Owned{};
        for (int32 Index = 0; Index < SlotCount; ++Index)
        {
            UObject* Slot{};
            if (!GetSlot.Set(STR("Index"), Index) || !GetSlot.Call() || !GetSlot.Get(STR("ReturnValue"), Slot) || !Slot)
            {
                continue;
            }

            bool IsEmpty{true};
            FuncCall Empty{};
            if (Empty.Bind(Slot, STR("IsEmpty")) && Empty.Call() && Empty.Get(STR("ReturnValue"), IsEmpty) && IsEmpty)
            {
                continue;
            }

            UObject* StaticItemData{};
            bool Resolved{};
            FuncCall GetStatic{};
            if (!GetStatic.Bind(Slot, STR("TryGetStaticItemData")) || !GetStatic.Call() ||
                !GetStatic.Get(STR("ReturnValue"), Resolved) || !Resolved ||
                !GetStatic.Get(STR("OutStaticItemData"), StaticItemData) || !StaticItemData)
            {
                continue;
            }

            FName ItemId{};
            if (!ReadProperty(StaticItemData, STR("ID"), ItemId))
            {
                continue;
            }

            uint8_t TypeB{};
            const auto HasTypeB = ReadProperty(StaticItemData, STR("TypeB"), TypeB);
            const auto IsConsumable = HasTypeB && TypeB == ConsumableKeyItemTypeB;

            if (!IsConsumable)
            {
                Owned.insert(ItemId.ToUnstableInt());
            }

            if (GetConfig().Verbosity() >= LogVerbosity::Discovery)
            {
                int32 MaxStackCount{};
                uint8_t TypeA{};
                bool NotConsumed{};
                ReadProperty(StaticItemData, STR("MaxStackCount"), MaxStackCount);
                ReadProperty(StaticItemData, STR("TypeA"), TypeA);
                ReadProperty(StaticItemData, STR("bNotConsumed"), NotConsumed);
                LogDiscovery(STR("Essential slot {} holds '{}' (TypeA={} TypeB={} MaxStack={} NotConsumed={}){}\n"),
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
            Log(STR("Key item inventory: {} distinct item(s)\n"), m_OwnedIds.size());
        }
        return true;
    }

    auto GetEssentialInventory() -> EssentialInventory&
    {
        static EssentialInventory Instance{};
        return Instance;
    }
} // namespace IAHO
