# I Already Have One

A UE4SS C++ mod for Palworld. Key items are crafted once and then live in their
own inventory, but crafting menus keep offering them with no sign you already
own one. This mod removes those recipes from the list.

## How it works

Palworld stores key items in a dedicated Essential container, separate from the
common inventory. That container *is* the "do I already have one" answer, so the
mod reads it and drops any recipe whose product is in it.

Everything is reached through UE reflection by name — no hardcoded struct
offsets — so a game patch that moves fields around does not silently corrupt
anything. The relevant game symbols are documented in
[`docs/game-api.md`](docs/game-api.md).

Three recipe-list functions are post-hooked, since which one feeds which
crafting surface is a runtime question:

| Function | Parameter filtered |
| --- | --- |
| `UPalUIUtility::FilteringWorkSpaceRecipe` | `OutFilteredArray` |
| `UPalTechnologyData::FilteringUnlockedRecipe` | `OutRecipeIdArray` |
| `UPalMapObjectConvertItemModel::GetRecipes` | `ReturnValue` |

Set `LogLevel = discovery` in `config.ini` to see which one fires for each menu.

The mod is client-side. On a dedicated server there is no local player state, so
it does nothing. Every failure path leaves the vanilla list untouched.

## Building

Requirements, per RE-UE4SS:

- Windows, MSVC toolset >= 14.43 (Visual Studio >= 17.13, "Desktop development with C++")
- Rust toolchain >= 1.73
- CMake >= 3.22 and Ninja
- A GitHub account linked to Epic Games, for the UEPseudo submodule

```powershell
git clone --recursive https://github.com/<you>/iaho.git
cd iaho
.\scripts\build.ps1 -Install
```

`deps/ue4ss` is pinned to the exact RE-UE4SS commit the installed loader was
built from (`ba2efd55`). C++ mods break if the loader and the mod disagree on
the C runtime or the UE4SS ABI, so re-pin it whenever you update UE4SS.

## Configuration

`config.ini` lands next to the dll's parent directory in
`ue4ss/Mods/IAlreadyHaveOne/`. See the comments in
[`mod/payload/config.ini`](mod/payload/config.ini).
