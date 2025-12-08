#!/usr/bin/env bash
set -euo pipefail

# Configuration
JACK_SOURCE=${1:-.}
BUILD_DIR="build"
SRC_DIR="$BUILD_DIR/src"     # copy .jack sources + OS here
COMPILER="./compiler/compiler"
VM_TRANSLATOR="./VM/hackVM2"
ASSEMBLER="./assembler/hackassembler"
OS_DIR="OS"
OUT_HACK="$BUILD_DIR/output.hack"

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

# Copy OS .jack files
if compgen -G "$OS_DIR/*.jack" > /dev/null; then
    find "$OS_DIR" -maxdepth 1 -type f -name "*.jack" -exec cp {} "$SRC_DIR" \;
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
if [[ ! -f "$ASM_FILE" ]]; then
    echo "Error: expected asm at $ASM_FILE not found" >&2
    exit 1
fi

echo "Assembling $ASM_FILE -> $OUT_HACK"
mkdir -p "$(dirname "$OUT_HACK")"
"$ASSEMBLER" "$ASM_FILE" "$OUT_HACK"

echo "Build complete: $OUT_HACK"
```// filepath: /home/filthyfil/Code/hack/build.sh
#!/usr/bin/env bash
set -euo pipefail

# Configuration
JACK_SOURCE=${1:-.}
BUILD_DIR="build"
SRC_DIR="$BUILD_DIR/src"     # copy .jack sources + OS here
COMPILER="./compiler/compiler"
VM_TRANSLATOR="./VM/hackVM2"
ASSEMBLER="./assembler/hackassembler"
OS_DIR="OS"
OUT_HACK="$BUILD_DIR/output.hack"

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

# Copy OS .jack files
if compgen -G "$OS_DIR/*.jack" > /dev/null; then
    find "$OS_DIR" -maxdepth 1 -type f -name "*.jack" -exec cp {} "$SRC_DIR" \;
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
if [[ ! -f "$ASM_FILE" ]]; then
    echo "Error: expected asm at $ASM_FILE not found" >&2
    exit 1
fi

echo "Assembling $ASM_FILE -> $OUT_HACK"
mkdir -p "$(dirname "$OUT_HACK")"
"$ASSEMBLER" "$ASM_FILE" "$OUT_HACK"

echo "Build complete: $OUT_HACK"