#!/usr/bin/env bash
# Valide un plugin avec pluginval (version figée, téléchargée dans tools/, ignorée par git).
# Usage : tools/pluginval.sh <chemin du .vst3 ou .component> [niveau, défaut 5]
set -euo pipefail

PLUGINVAL_VERSION="v1.0.4"
TOOLS_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$TOOLS_DIR/pluginval.app/Contents/MacOS/pluginval"
PLUGIN="${1:?chemin du plugin attendu}"
LEVEL="${2:-5}"

if [[ ! -x "$BIN" ]]; then
    echo "Téléchargement de pluginval $PLUGINVAL_VERSION..."
    curl -fsSL -o "$TOOLS_DIR/pluginval_macOS.zip" \
        "https://github.com/Tracktion/pluginval/releases/download/$PLUGINVAL_VERSION/pluginval_macOS.zip"
    unzip -q -o "$TOOLS_DIR/pluginval_macOS.zip" -d "$TOOLS_DIR"
    rm "$TOOLS_DIR/pluginval_macOS.zip"
fi

"$BIN" --strictness-level "$LEVEL" --validate-in-process --skip-gui-tests --timeout-ms 300000 --validate "$PLUGIN"
