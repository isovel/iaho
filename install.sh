#!/usr/bin/env bash
# Deploy the built mod into Palworld's UE4SS Mods folder.
#   ue4ss/Mods/IAlreadyHaveOne/enabled.txt
#   ue4ss/Mods/IAlreadyHaveOne/dlls/main.dll
#   ue4ss/Mods/IAlreadyHaveOne/config.ini   (seeded on first install only)
# Override the destination with UE4SS_MODS_DIR.
set -euo pipefail

MOD_NAME="IAlreadyHaveOne"
SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODS_DIR="${UE4SS_MODS_DIR:-/mnt/s/SteamLibrary/steamapps/common/Palworld/Pal/Binaries/Win64/ue4ss/Mods}"
DEST="$MODS_DIR/$MOD_NAME"

[[ -f "$SRC_DIR/dlls/main.dll" ]] || { echo "install: dlls/main.dll not found - build first" >&2; exit 1; }
[[ -d "$MODS_DIR" ]] || { echo "install: mods dir not found: $MODS_DIR" >&2; exit 1; }

mkdir -p "$DEST/dlls"

# Copy straight over the target: renaming onto an existing file fails on WSL
# drvfs mounts. Windows locks main.dll while the game runs, so a lock fails the
# open() up front rather than leaving a truncated DLL behind.
if ! cp -f "$SRC_DIR/dlls/main.dll" "$DEST/dlls/main.dll" 2>/dev/null; then
    echo "install: could not write $DEST/dlls/main.dll (is Palworld running?)" >&2
    exit 1
fi

rm -f "$DEST/dlls/main.pdb"
touch "$DEST/enabled.txt"

# Seed the config on first install only; never clobber edits made in the game folder
if [[ ! -f "$DEST/config.ini" ]]; then
    cp "$SRC_DIR/mod/payload/config.ini" "$DEST/config.ini"
    echo "install: seeded config.ini"
fi

echo "install: deployed $MOD_NAME -> $DEST"
