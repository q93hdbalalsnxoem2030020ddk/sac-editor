#!/bin/bash

BIN_NAME="sac++"
SOURCE_PATH="$(realpath "$BIN_NAME" 2>/dev/null || readlink -f "$BIN_NAME" 2>/dev/null || echo "$BIN_NAME")"
DEST=""

if [ ! -f "$SOURCE_PATH" ]; then
    exit 1
fi

if [[ "$PREFIX" == *"com.termux"* ]] || [[ "$HOME" == *"/data/data/com.termux/files/home"* ]]; then
    DEST="$PREFIX/bin"
    mkdir -p "$DEST" 2>/dev/null
    cp "$SOURCE_PATH" "$DEST/" 2>/dev/null
    chmod +x "$DEST/$BIN_NAME" 2>/dev/null
elif [ -d "/usr/local/bin" ] && [ -w "/usr/local/bin" ]; then
    DEST="/usr/local/bin"
    sudo mkdir -p "$DEST" 2>/dev/null
    sudo cp "$SOURCE_PATH" "$DEST/" 2>/dev/null
    sudo chmod +x "$DEST/$BIN_NAME" 2>/dev/null
else
    DEST="$HOME/.local/bin"
    mkdir -p "$DEST" 2>/dev/null
    cp "$SOURCE_PATH" "$DEST/" 2>/dev/null
    chmod +x "$DEST/$BIN_NAME" 2>/dev/null
    if [ -f "$DEST/$BIN_NAME" ] && [ -x "$DEST/$BIN_NAME" ]; then
        if ! echo "$PATH" | grep -q "$DEST"; then
            SHELL_CONFIG=""
            if [ -n "$ZSH_VERSION" ]; then
                SHELL_CONFIG="$HOME/.zshrc"
            elif [ -f "$HOME/.bashrc" ]; then
                SHELL_CONFIG="$HOME/.bashrc"
            elif [ -f "$HOME/.profile" ]; then
                SHELL_CONFIG="$HOME/.profile"
            else
                SHELL_CONFIG="$HOME/.profile"
                touch "$SHELL_CONFIG" 2>/dev/null
            fi
            echo "export PATH=\"\$PATH:$DEST\"" >> "$SHELL_CONFIG" 2>/dev/null
        fi
        if ! command -v "$BIN_NAME" >/dev/null 2>&1; then
            DEST="$HOME/bin"
            mkdir -p "$DEST" 2>/dev/null
            cp "$SOURCE_PATH" "$DEST/" 2>/dev/null
            chmod +x "$DEST/$BIN_NAME" 2>/dev/null
            if ! echo "$PATH" | grep -q "$DEST"; then
                SHELL_CONFIG=""
                if [ -n "$ZSH_VERSION" ]; then
                    SHELL_CONFIG="$HOME/.zshrc"
                elif [ -f "$HOME/.bashrc" ]; then
                    SHELL_CONFIG="$HOME/.bashrc"
                elif [ -f "$HOME/.profile" ]; then
                    SHELL_CONFIG="$HOME/.profile"
                else
                    SHELL_CONFIG="$HOME/.profile"
                    touch "$SHELL_CONFIG" 2>/dev/null
                fi
                echo "export PATH=\"\$PATH:$DEST\"" >> "$SHELL_CONFIG" 2>/dev/null
            fi
        fi
    fi
fi

if [ -f "$DEST/$BIN_NAME" ] && [ -x "$DEST/$BIN_NAME" ]; then
    if command -v "$BIN_NAME" >/dev/null 2>&1; then
        exit 0
    else
        exit 1
    fi
else
    exit 1
fi
