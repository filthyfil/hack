#!/usr/bin/env bash
set -euo pipefail

# Configuration
JACK_SOURCE=${1:-.}
BUILD_DIR="build"
SRC_DIR="$BUILD_DIR/src"
COMPILER="./j"
VM_TRANSLATOR="./VM/VirtualMachine"
ASSEMBLER="./assembler/Assembler"
OS_DIR="OS"
OUT_HACK="$BUILD_DIR/o.hack"

clean() {
    echo "Cleaning $BUILD_DIR"
    rm -rf "$BUILD_DIR"
}

if [[ "${JACK_SOURCE}" == "clean" ]]; then
    clean
    exit 0
fi

if [[ ! -e "$JACK_SOURCE" ]]; then
    echo "Error: source '$JACK_SOURCE' not found" >&2
    exit 1
fi

mkdir -p "$SRC_DIR"
rm -f "$SRC_DIR"/*

# Copy only .jack files from program source (file or dir)
if [[ -f "$JACK_SOURCE" && "${JACK_SOURCE##*.}" == "jack" ]]; then
    cp "$JACK_SOURCE" "$SRC_DIR/"
else
    find "$JACK_SOURCE" -maxdepth 1 -type f -name "*.jack" -exec cp {} "$SRC_DIR" \;
fi

# Optionally copy pre-compiled OS .vm files (skip .jack compilation).
# Set INCLUDE_OS=0 in the environment to skip copying the OS and produce
# a smaller output (useful when you only want to build a small program).
INCLUDE_OS=${INCLUDE_OS:-1}
if [[ "$INCLUDE_OS" -ne 0 ]]; then
    if compgen -G "$OS_DIR/*.vm" > /dev/null; then
        find "$OS_DIR" -maxdepth 1 -type f -name "*.vm" -exec cp {} "$SRC_DIR" \;
    fi
else
    echo "Skipping OS .vm files (INCLUDE_OS=0)"
fi

# Ensure tools exist
if [[ ! -x "$COMPILER" ]]; then echo "Error: compiler not found/executable ($COMPILER)" >&2; exit 1; fi
if [[ ! -x "$VM_TRANSLATOR" ]]; then echo "Error: VM translator not found/executable ($VM_TRANSLATOR)" >&2; exit 1; fi
if [[ ! -x "$ASSEMBLER" ]]; then echo "Error: assembler not found/executable ($ASSEMBLER)" >&2; exit 1; fi

echo "Compiling Jack sources in $SRC_DIR -> .vm"
"$COMPILER" "$SRC_DIR"

echo "Translating VM -> ASM (feeding translator directory $SRC_DIR)"
"$VM_TRANSLATOR" "$SRC_DIR"

ASM_FILE="$SRC_DIR/$(basename "$SRC_DIR").asm"
ASM_TO_ASSEMBLE="$ASM_FILE"
if [[ ! -f "$ASM_FILE" ]]; then
    ASM_FILE=$(find "$SRC_DIR" -maxdepth 1 -name "*.asm" | head -n1)
    ASM_TO_ASSEMBLE="$ASM_FILE"
fi

echo "Assembling $ASM_TO_ASSEMBLE"
mkdir -p "$(dirname "$OUT_HACK")"
"$ASSEMBLER" "$ASM_TO_ASSEMBLE"

# Copy the generated .hack file to BUILD_DIR
# The assembler creates a .hack file with the same name as the input .asm file
ASM_BASENAME=$(basename "$ASM_TO_ASSEMBLE" .asm)
GENERATED_HACK="$SRC_DIR/$ASM_BASENAME.hack"
if [[ -f "$GENERATED_HACK" ]]; then
    cp "$GENERATED_HACK" "$OUT_HACK"
elif [[ -f "$SRC_DIR/output.hack" ]]; then
    # Fallback to old output.hack name
    cp "$SRC_DIR/output.hack" "$OUT_HACK"
fi

echo "Build complete: $OUT_HACK"