# Hack VM Translator

This directory contains an improved implementation of the Hack VM Translator for the Nand2Tetris project, along with a user-friendly Streamlit web application.

## Files

- **`hackVM2.cpp`** - Improved VM translator with readable assembly output
- **`vm_translator_app.py`** - Streamlit web application for VM translation
- **`requirements.txt`** - Python dependencies for the Streamlit app

## Features

### Improved Assembly Output
The `hackVM2.cpp` implementation now generates much more readable assembly code:
- Each assembly instruction appears on its own line
- Clear separation between different operations
- Better formatting for debugging and understanding

### Streamlit Web App
A user-friendly web interface that allows you to:
- Input VM code directly in the browser
- See real-time translation to assembly
- Download the generated assembly file
- Use pre-built examples
- View statistics about the translation

## Usage

### Command Line Usage

1. **Compile the translator:**
   ```bash
   g++ -o hackVM2 hackVM2.cpp
   ```

2. **Translate a VM file:**
   ```bash
   ./hackVM2 input.vm output.asm
   ```

### Web App Usage

1. **Install dependencies:**
   ```bash
   pip install -r requirements.txt
   ```

2. **Run the Streamlit app:**
   ```bash
   streamlit run vm_translator_app.py
   ```

3. **Open your browser** to the URL shown in the terminal (usually `http://localhost:8501`)

4. **Use the app:**
   - Enter VM code in the left panel
   - Click "Translate" to see the assembly output
   - Use the sidebar examples to get started
   - Download the generated assembly file

## Supported VM Commands

### Stack Arithmetic
- `add`, `sub`, `neg`, `eq`, `gt`, `lt`, `and`, `or`, `not`

### Memory Access
- `push segment index` - Push value from segment[index] to stack
- `pop segment index` - Pop value from stack to segment[index]

**Supported segments:**
- `constant` - Push constant value
- `local` - Local variables (LCL base)
- `argument` - Function arguments (ARG base)
- `this` - Current object (THIS base)
- `that` - Other object (THAT base)
- `temp` - Temporary variables (RAM[5]-RAM[12])
- `pointer` - THIS/THAT pointers (RAM[3], RAM[4])
- `static` - Static variables (file-specific)

### Program Flow
- `label labelName` - Define a label
- `goto labelName` - Unconditional jump
- `if-goto labelName` - Conditional jump (if top of stack ≠ 0)

### Function Calls
- `function functionName nLocals` - Define function with n local variables
- `call functionName nArgs` - Call function with n arguments
- `return` - Return from function

## Example VM Code

```vm
// Simple arithmetic
push constant 7
push constant 8
add
push constant 15
sub

// Comparison
push constant 10
push constant 5
gt

// Function definition
function mult 2
    push argument 0
    push argument 1
    call Math.multiply 2
    return
```

## Architecture

The translator follows the standard VM Translator architecture:

1. **Parser** - Reads and parses VM commands
2. **CodeWriter** - Generates corresponding assembly code
3. **Memory Management** - Handles different memory segments
4. **Function Protocol** - Manages function calls and returns

## Memory Segments

- **LCL (Local)** - Points to current function's local variables
- **ARG (Argument)** - Points to current function's arguments  
- **THIS/THAT** - Object references
- **SP (Stack Pointer)** - Always points to next free stack location
- **TEMP** - Fixed RAM locations (5-12) for temporary storage
- **STATIC** - File-specific variables

## Benefits of the Improved Implementation

1. **Readability** - Each assembly instruction on its own line
2. **Debugging** - Easier to trace through generated code
3. **Maintenance** - Clearer code structure
4. **Learning** - Better for understanding the translation process
5. **Web Interface** - User-friendly translation without command line

## Troubleshooting

- **Compilation errors**: Ensure you have g++ installed
- **Translation errors**: Check VM code syntax
- **Web app issues**: Verify Python and Streamlit are installed correctly

## Contributing

Feel free to improve the code or add new features. The goal is to make VM translation as clear and accessible as possible for learning and development.
