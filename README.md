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

One function is post-hooked: `UPalMapObjectConvertItemModel::GetRecipes`, whose
return value is the list every crafting surface reads. Two other candidates were
hooked during bring-up and dropped — `UPalUIUtility::FilteringWorkSpaceRecipe`
returned an empty array on all eight calls, and
`UPalTechnologyData::FilteringUnlockedRecipe` never fired.

The container also holds consumables, which are used up and worth crafting
again, so those are skipped. `TypeB` makes the split: the consumables sit in the
generic `Essential` bucket (54) while every craft-once item has a dedicated one —
`Essential_UnlockPlayerFuture`, `Essential_PalGear`,
`Essential_AdditionalInventory`, `Essential_Lamp`, `Blueprint`. See
[`docs/game-api.md`](docs/game-api.md) for the measured values.

Set `LogLevel = discovery` in `config.ini` to trace every call and decision.

The mod is client-side. On a dedicated server there is no local player state, so
it does nothing. Every failure path leaves the vanilla list untouched.

## Build (Linux → Windows cross-compile)

Toolchain: clang-cl + [xwin](https://github.com/Jake-Shadle/xwin), per the
[UE4SS docs](https://docs.ue4ss.com/index.html#cross-compiling-windows-binaries-on-linux).
Same setup as the neighbouring PerkyPals mod.

One-time setup:

```
sudo apt-get install -y build-essential cmake ninja-build
wget -qO /tmp/llvm.sh https://apt.llvm.org/llvm.sh && chmod +x /tmp/llvm.sh && sudo /tmp/llvm.sh 19 all
curl -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal --target x86_64-pc-windows-msvc
xwin --accept-license --arch x86_64 --variant desktop splat --output ~/.xwin
```

Then:

```
git clone --recursive <this repo> && cd iaho
./build-linux.sh                        # Game__Shipping__Win64
NO_INSTALL=1 ./build-linux.sh           # skip the deploy step
```

Output: `dlls/main.dll`, PE32+ x86-64 exporting `start_mod` / `uninstall_mod`.
Set `XWIN_DIR` or `LLVM_ROOT` to override the default locations, and
`UE4SS_MODS_DIR` to point `install.sh` at another game install.

Two cross-compile quirks are handled in `CMakeLists.txt` and `build-linux.sh`:

- clang-cl has no `/std:c++23`, so CMake maps `CXX_STANDARD 23` to
  `/std:c++latest` (C++26), where UE4SS's Unreal headers hit P2864 (bitwise ops
  between different enum types = hard error). The standard flag is overridden to
  a real `-std=c++23`.
- The build makes only the `IAlreadyHaveOne` target. A full `cmake --build` also
  builds UE4SS's `proxy_generator`, a Windows .exe the build then tries to run
  on the host ("Exec format error").

## Build (Windows, MSVC)

Requires MSVC toolset >= 14.43 (Visual Studio >= 17.13, "Desktop development
with C++"), Rust >= 1.73, CMake >= 3.22.

```
git submodule update --init --recursive
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Game__Shipping__Win64 --target IAlreadyHaveOne
```

Valid configs: `Game__Shipping__Win64`, `Game__Debug__Win64`,
`Game__Development__Win64`.

`deps/ue4ss` is pinned to the exact RE-UE4SS commit the installed loader was
built from (`ba2efd55`). C++ mods break if the loader and the mod disagree on
the C runtime or the UE4SS ABI, so re-pin it whenever you update UE4SS.

## Install

`./install.sh` (run for you by `build-linux.sh`) produces:

```
<Game>/Pal/Binaries/Win64/ue4ss/Mods/IAlreadyHaveOne/dlls/main.dll
<Game>/Pal/Binaries/Win64/ue4ss/Mods/IAlreadyHaveOne/enabled.txt
<Game>/Pal/Binaries/Win64/ue4ss/Mods/IAlreadyHaveOne/config.ini
```

`enabled.txt` alone is enough for UE4SS to start the mod, leaving `mods.json`
and `mods.txt` untouched. Close the game first; Windows locks `main.dll` while
it runs. `config.ini` is seeded on first install only, so edits in the game
folder survive a rebuild.

## Configuration

Every setting is documented in [`mod/payload/config.ini`](mod/payload/config.ini),
which is what gets seeded into the game folder.
