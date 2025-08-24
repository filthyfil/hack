# Hack Computer Development Tools - Streamlit Apps

This directory contains user-friendly web applications for the Hack Computer development tools, built with Streamlit.

## 🚀 Quick Start

### Installation

1. **Install dependencies:**
   ```bash
   cd streamlit_apps
   pip install -r requirements.txt
   ```

### Running the Applications

#### Option 1: Main Dashboard (Recommended)
```bash
streamlit run main_app.py
```
This launches a dashboard where you can access all tools.

#### Option 2: Individual Apps
```bash
# Assembler only
streamlit run assembler_app.py

# VM Translator only  
streamlit run vm_translator_app.py
```

### Network Access
To access from other devices on your network:
```bash
streamlit run main_app.py --server.address 0.0.0.0 --server.port 8501
```

## 📱 Applications

### 🖥️ Main Dashboard (`main_app.py`)
- **Purpose**: Central hub for all Hack Computer tools
- **Features**: 
  - Overview of the Hack Computer architecture
  - Quick access to all tools
  - Development workflow guidance
  - Technical specifications

### ⚙️ Hack Assembler (`assembler_app.py`)
- **Purpose**: Convert Hack Assembly code to machine language
- **Input**: Assembly code (.asm format)
- **Output**: 16-bit machine code (.hack format)
- **Features**:
  - Real-time assembly translation
  - Code analysis and statistics
  - Multiple output formats (clean/detailed)
  - Built-in examples (basic programs, loops, screen manipulation)
  - Syntax validation and error reporting
  - Download functionality

### 🔄 VM Translator (`vm_translator_app.py`)
- **Purpose**: Translate VM code to Hack Assembly
- **Input**: Virtual Machine code (.vm format)
- **Output**: Assembly code (.asm format)
- **Features**:
  - Real-time VM to Assembly translation
  - Support for all VM commands (arithmetic, memory access, flow control)
  - Function call protocols
  - Multiple output formats (clean/detailed)
  - Built-in examples (arithmetic, comparisons, function calls)
  - Statistics and analysis

## 🛠️ Supported Operations

### Assembler
- **A-instructions**: `@value` or `@symbol`
- **C-instructions**: `dest=comp;jump`
- **Labels**: `(LABEL)`
- **Comments**: `// comment text`
- **Predefined symbols**: R0-R15, SP, LCL, ARG, THIS, THAT, SCREEN, KBD

### VM Translator
- **Stack arithmetic**: add, sub, neg, eq, gt, lt, and, or, not
- **Memory access**: push/pop for all segments (local, argument, this, that, constant, temp, pointer, static)
- **Program flow**: label, goto, if-goto
- **Function calls**: function, call, return

## 📁 File Structure

```
streamlit_apps/
├── main_app.py           # Main dashboard
├── assembler_app.py      # Assembler web interface
├── vm_translator_app.py  # VM Translator web interface
├── requirements.txt      # Python dependencies
└── README.md            # This file
```

## 🔧 How It Works

### Assembler App
1. **Compilation**: Automatically compiles `../assembler/hackassembler.cpp`
2. **Processing**: Creates temporary files for input/output
3. **Assembly**: Runs the compiled assembler on the input
4. **Display**: Shows formatted machine code with annotations

### VM Translator App
1. **Compilation**: Automatically compiles `../VM/hackVM2.cpp`
2. **Processing**: Creates temporary files for input/output
3. **Translation**: Runs the compiled translator on the input
4. **Display**: Shows formatted assembly code with annotations

## 🎨 Features

### User Interface
- **Clean, modern design** with intuitive layouts
- **Responsive columns** for input and output
- **Real-time processing** with status indicators
- **Multiple format options** for different use cases
- **Built-in examples** for learning and testing

### Code Analysis
- **Statistics**: Line counts, instruction types, symbol usage
- **Validation**: Syntax checking and error reporting
- **Formatting**: Clean or detailed output with annotations
- **Download**: Save generated files locally

### Educational Value
- **Examples**: Pre-built code samples for learning
- **Annotations**: Detailed explanations of generated code
- **Workflow guidance**: Step-by-step development process
- **Architecture overview**: Understanding the Hack platform

## 🚨 Troubleshooting

### Common Issues

1. **Compilation errors**: 
   - Ensure g++ is installed
   - Check that the C++ source files exist in the correct directories

2. **Path issues**:
   - Run from the `streamlit_apps` directory
   - Ensure the parent directory contains the `assembler/` and `VM/` folders

3. **Permission errors**:
   - Ensure write permissions for temporary files
   - Check executable permissions for compiled binaries

### Error Messages
- **"Compilation failed"**: C++ source code has syntax errors
- **"Assembly/Translation failed"**: Input code has syntax errors
- **"File not found"**: Incorrect paths or missing source files

## 🔗 Integration

These web apps integrate seamlessly with the existing C++ implementations:
- **Assembler**: Uses `../assembler/hackassembler.cpp`
- **VM Translator**: Uses `../VM/hackVM2.cpp`

The apps handle compilation automatically and provide user-friendly interfaces to the underlying tools.

## 📚 Learning Resources

Use these apps to:
1. **Understand Assembly**: See how high-level constructs map to machine code
2. **Learn VM Architecture**: Understand stack-based computation
3. **Debug Programs**: Analyze generated code step by step
4. **Experiment**: Try different coding patterns and see results
5. **Build Intuition**: Develop understanding of computer systems

## 🎯 Use Cases

### Educational
- **Teaching**: Demonstrate computer architecture concepts
- **Learning**: Hands-on experience with low-level programming
- **Research**: Experiment with different programming approaches

### Development
- **Prototyping**: Quick testing of assembly/VM code
- **Debugging**: Analyze generated code for optimization
- **Validation**: Verify correct translation of high-level constructs

### Professional
- **Documentation**: Generate examples for technical writing
- **Presentations**: Live demonstrations of compilation process
- **Training**: Onboard new developers to systems programming

---

**Note**: These applications require the corresponding C++ source files to be present in the parent directory structure. They provide web-based interfaces to the command-line tools while maintaining full compatibility with the original implementations.
