import streamlit as st
import subprocess
import os

def main():
    st.set_page_config(
        page_title="Hack Computer Tools",
        page_icon="🖥️",
        layout="wide"
    )
    
    st.title("🖥️ Hack Computer Development Tools")
    st.markdown("A comprehensive toolkit for the Nand2Tetris Hack Computer platform")
    
    # Introduction
    st.markdown("""
    Welcome to the Hack Computer Development Tools! This suite provides user-friendly web interfaces 
    for the essential tools needed for Hack computer development.
    """)
    
    # Tools overview
    col1, col2 = st.columns(2)
    
    with col1:
        st.markdown("""
        ### ⚙️ Hack Assembler
        Convert Hack Assembly language to machine code.
        
        **Features:**
        - Real-time assembly translation
        - Code analysis and statistics
        - Multiple output formats
        - Built-in examples
        - Syntax validation
        
        **Use Cases:**
        - Convert `.asm` files to `.hack` files
        - Learn assembly programming
        - Debug assembly code
        - Understand machine code generation
        """)
        
        if st.button("🚀 Launch Assembler", type="primary", use_container_width=True):
            # MODIFICATION: Added 'pages/' to the path
            st.switch_page("pages/assembler_app.py")
    
    with col2:
        st.markdown("""
        ### 🔄 VM Translator
        Translate Virtual Machine code to Hack Assembly.
        
        **Features:**
        - VM to Assembly translation
        - Support for all VM commands
        - Function call protocols
        - Program flow control
        - Memory segment management
        
        **Use Cases:**
        - Convert `.vm` files to `.asm` files
        - Understand VM architecture
        - Debug VM programs
        - Learn compilation techniques
        """)
        
        if st.button("🚀 Launch VM Translator", type="primary", use_container_width=True):
            # MODIFICATION: Added 'pages/' to the path
            st.switch_page("pages/vm_translator_app.py")
    
    # Architecture overview
    st.markdown("---")
    st.header("🏗️ Hack Computer Architecture")
    
    col1, col2, col3 = st.columns(3)
    
    with col1:
        st.markdown("""
        #### High-Level Language
        ```
        Jack/Java-like code
        ↓
        ```
        """)
    
    with col2:
        st.markdown("""
        #### Virtual Machine
        ```
        VM commands
        ↓ (VM Translator)
        ```
        """)
    
    with col3:
        st.markdown("""
        #### Machine Language
        ```
        Assembly code
        ↓ (Assembler)
        Machine code
        ```
        """)
    
    # Workflow
    st.markdown("---")
    st.header("🔄 Development Workflow")
    
    workflow_steps = [
        ("1. Write High-Level Code", "Create programs in Jack or other high-level languages"),
        ("2. Compile to VM Code", "Generate VM commands from high-level code"),
        ("3. Translate VM to Assembly", "Use the VM Translator to convert VM code to assembly"),
        ("4. Assemble to Machine Code", "Use the Assembler to generate 16-bit machine code"),
        ("5. Run on Hack Computer", "Execute the machine code on the Hack platform")
    ]
    
    for i, (title, description) in enumerate(workflow_steps):
        with st.container():
            col1, col2 = st.columns([1, 4])
            with col1:
                st.markdown(f"**{title}**")
            with col2:
                st.markdown(description)
    
    # Technical specifications
    st.markdown("---")
    st.header("⚙️ Technical Specifications")
    
    col1, col2 = st.columns(2)
    
    with col1:
        st.markdown("""
        #### Assembler Specifications
        - **Input**: Hack Assembly (.asm)
        - **Output**: Machine Code (.hack)
        - **Instruction Set**: A-instructions, C-instructions
        - **Symbols**: Predefined + user-defined
        - **Memory**: 32K RAM + memory-mapped I/O
        """)
    
    with col2:
        st.markdown("""
        #### VM Translator Specifications
        - **Input**: VM Code (.vm)
        - **Output**: Assembly Code (.asm)
        - **Commands**: Arithmetic, memory access, flow control
        - **Functions**: Call/return protocol
        - **Memory Segments**: Local, argument, static, etc.
        """)
    
    # Quick start
    st.markdown("---")
    st.header("🚀 Quick Start")
    
    with st.expander("Getting Started with the Assembler", expanded=False):
        st.markdown("""
        1. **Click "Launch Assembler"** above
        2. **Try an example** from the sidebar
        3. **Write your assembly code** in the left panel
        4. **Click "Assemble"** to generate machine code
        5. **Download the .hack file** for use in the Hack computer
        
        **Example Assembly Code:**
        ```assembly
        @2      // Load constant 2
        D=A     // D = 2
        @3      // Load constant 3  
        D=D+A   // D = 2 + 3
        @sum    // Load address of sum
        M=D     // Store result in sum
        ```
        """)
    
    with st.expander("Getting Started with the VM Translator", expanded=False):
        st.markdown("""
        1. **Click "Launch VM Translator"** above
        2. **Try an example** from the sidebar
        3. **Write your VM code** in the left panel
        4. **Click "Translate"** to generate assembly code
        5. **Download the .asm file** for further assembly
        
        **Example VM Code:**
        ```vm
        push constant 7
        push constant 8
        add
        pop static 0
        ```
        """)
    
    # Footer
    st.markdown("---")
    st.markdown("""
    **Hack Computer Development Tools** - Part of the Nand2Tetris project.
    
    These tools implement the software layer of the Hack computer platform, providing 
    a complete development environment for understanding computer systems from first principles.
    """)

if __name__ == "__main__":
    main()
