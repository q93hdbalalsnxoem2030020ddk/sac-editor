#!/bin/bash

# sac++ installer
BIN_NAME="sac++"
SOURCE_PATH="$(realpath "$BIN_NAME")"

if [[ "$PREFIX" == *"com.termux"* ]] || [[ "$HOME" == *"/data/data/com.termux/files/home"* ]]; then
    DEST="$PREFIX/bin"
    cp "$SOURCE_PATH" "$DEST/"
    chmod +x "$DEST/$BIN_NAME"
    echo "installed to Termux: $DEST/$BIN_NAME"
elif [ -d "/usr/local/bin" ] && [ -w "/usr/local/bin" ]; then
    sudo cp "$SOURCE_PATH" /usr/local/bin/
    sudo chmod +x /usr/local/bin/$BIN_NAME
    echo "installed to Linux: /usr/local/bin/$BIN_NAME"
else
    TARGET="$HOME/.local/bin"
    mkdir -p "$TARGET"
    cp "$SOURCE_PATH" "$TARGET/"
    chmod +x "$TARGET/$BIN_NAME"
    if ! echo "$PATH" | grep -q "$TARGET"; then
        if [ -f "$HOME/.bashrc" ]; then
            echo "export PATH=\"\$PATH:$TARGET\"" >> "$HOME/.bashrc"
        elif [ -f "$HOME/.profile" ]; then
            echo "export PATH=\"\$PATH:$TARGET\"" >> "$HOME/.profile"
        fi
    fi
    echo "installed to fallback: $TARGET/$BIN_NAME"
fi