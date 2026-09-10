# Palworld symbols used by this mod

Line numbers refer to the UE4SS reflection dump at
`Pal/Binaries/Win64/ue4ss/CXXHeaderDump/Pal.hpp` for the build dated 2026-09-08.
Everything listed is a reflected `UFunction` or `UProperty`, so it is reachable
by name at runtime.

## Reading the key item inventory

| Purpose | Symbol | Line |
| --- | --- | --- |
| Local player state | `UPalUtility::GetLocalPlayerState(WorldContextObject)` | 38137 |
| Inventory data | `APalPlayerState::GetInventoryData()` | 13403 |
| Essential container | `UPalPlayerInventoryData::TryGetContainerFromInventoryType(inventoryType, OutContainer)` | 31948 |
| Slot count | `UPalItemContainer::Num()` | 25432 |
| Slot by index | `UPalItemContainer::Get(Index)` | 25439 |
| Slot occupancy | `UPalItemSlot::IsEmpty()` | 25584 |
| Slot item data | `UPalItemSlot::TryGetStaticItemData(OutStaticItemData)` | 25573 |
| Item id | `UPalStaticItemDataBase::ID` | 34854 |
| Craft-once test | `UPalStaticItemDataBase::MaxStackCount` | 34862 |

`EPalPlayerInventoryType::Essential == 2` (`Pal_enums.hpp:4441`).

`UPalItemContainer::GetItemStackCount(StaticItemId)` (line 25437) is a cheaper
single-item alternative when a full snapshot is not needed.

Container membership alone is not enough: consumables such as `KeySphere_*`,
`PalSummon_*` and `Salvage_TreasureBoxKey*` live in the Essential container too.
`MaxStackCount == 1` separates the craft-once unlocks from them.

## The crafting menu list

`UPalMapObjectConvertItemModel::GetRecipes()` (line 26888) returns the recipe id
list every crafting surface displays; its backing field is `RecipeIds` at
`+0x2A8`. Measured in game: 124 entries at the Pal gear bench, 545 at the
workbench.

Two other candidates were hooked during bring-up and removed.
`UPalUIUtility::FilteringWorkSpaceRecipe` (line 37465) fired eight times and
returned an empty array every time; `UPalTechnologyData::FilteringUnlockedRecipe`
(line 35463) never fired.

## Recipes

`FPalItemRecipe` (line 4428) is the recipe row; `Product_Id` names the item it
yields. Rows are reached through
`UPalMasterDataTablesUtility::GetItemRecipeDataTableAccess(WorldContextObject)`
(line 29712) and then
`UPalMasterDataTableAccess_ItemRecipe::BP_FindRow(RowName, bResult)` (line 29494).

The mod reads `Product_Id`'s offset from the `FPalItemRecipe` script struct
rather than assuming `0x08`.

## Alternatives considered

**Blocking rather than hiding.** `UPalTechnologyData` (line 35411) carries
`DenyRecipeList` and `IsDeniedRecipe(RecipeID)` (line 35454), seeded from the
world option `FPalOptionWorldSettings::DenyTechnologyList` (line 6318). This is
the game's own supported "this recipe is off" switch, and post-hooking
`IsDeniedRecipe` to return true would block crafting outright. Not used: hiding
was the goal, and denial is server-authoritative.

**A filter toggle in the crafting menu.** No such widget exists.
`FPalItemContainerFilter` and `UPalItemContainer::GetFilterOffList` are storage
container filters, unrelated to crafting. Adding a toggle would mean shipping a
modified `WBP` inside a `.pak`, which is far more fragile across game updates
than a list post-hook. `config.ini` covers the same need.
