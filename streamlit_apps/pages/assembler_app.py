import streamlit as st
import subprocess
import tempfile
import os
import re

def compile_assembler():
    """Compile the assembler C++ code"""
    try:
        result = subprocess.run(
            ["g++", "-o", "assembler/hackassembler", "assembler/hackassembler.cpp"],
            capture_output=True,
            text=True,
            cwd=".."
        )
        if result.returncode != 0:
            st.error(f"Compilation failed: {result.stderr}")
            return False
        return True
    except Exception as e:
        st.error(f"Compilation error: {str(e)}")
        return False

def assemble_hack_code(asm_code, filename="input"):
    """Assemble Hack assembly code to machine code using the compiled assembler"""
    try:
        # Create temporary files
        with tempfile.NamedTemporaryFile(mode='w', suffix='.asm', delete=False) as asm_file:
            asm_file.write(asm_code)
            asm_file_path = asm_file.name
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.hack', delete=False) as hack_file:
            hack_file_path = hack_file.name
        
        # Run the assembler
        result = subprocess.run(
            ["../assembler/hackassembler", asm_file_path, hack_file_path],
            capture_output=True,
            text=True,
            cwd="."
        )
        
        # Clean up temp input file
        os.unlink(asm_file_path)
        
        if result.returncode != 0:
            st.error(f"Assembly failed: {result.stderr}")
            return None
        
        # Read the output
        with open(hack_file_path, 'r') as f:
            machine_output = f.read()
        
        # Clean up output file
        os.unlink(hack_file_path)
        
        return machine_output
        
    except Exception as e:
        st.error(f"Assembly error: {str(e)}")
        return None

def format_machine_code(machine_code):
    """Format machine code for better display"""
    lines = machine_code.strip().split('\n')
    formatted_lines = []
    
    for i, line in enumerate(lines, 1):
        if line.strip():
            # Add line numbers and format binary
            if len(line.strip()) == 16 and line.strip().replace('0', '').replace('1', '') == '':
                # It's a 16-bit binary instruction
                binary = line.strip()
                decimal = int(binary, 2)
                hex_val = hex(decimal)[2:].upper().zfill(4)
                formatted_lines.append(f"{i:4d}: {binary}  // Dec: {decimal:5d}, Hex: 0x{hex_val}")
            else:
                formatted_lines.append(f"{i:4d}: {line.strip()}")
    
    return '\n'.join(formatted_lines)

def format_machine_code_detailed(machine_code):
    """Format machine code with instruction type annotations"""
    lines = machine_code.strip().split('\n')
    formatted_lines = []
    
    for i, line in enumerate(lines, 1):
        if line.strip():
            binary = line.strip()
            if len(binary) == 16 and binary.replace('0', '').replace('1', '') == '':
                decimal = int(binary, 2)
                hex_val = hex(decimal)[2:].upper().zfill(4)
                
                # Determine instruction type
                if binary[0] == '0':
                    # A-instruction
                    address = int(binary[1:], 2)
                    formatted_lines.append(f"{i:4d}: {binary}  // A-instruction: @{address} (Dec: {decimal}, Hex: 0x{hex_val})")
                else:
                    # C-instruction
                    comp = binary[3:10]
                    dest = binary[10:13]
                    jump = binary[13:16]
                    formatted_lines.append(f"{i:4d}: {binary}  // C-instruction: comp={comp} dest={dest} jump={jump} (Dec: {decimal}, Hex: 0x{hex_val})")
            else:
                formatted_lines.append(f"{i:4d}: {line.strip()}")
    
    return '\n'.join(formatted_lines)

def analyze_assembly_code(asm_code):
    """Analyze assembly code and provide statistics"""
    lines = asm_code.strip().split('\n')
    
    stats = {
        'total_lines': len(lines),
        'code_lines': 0,
        'comment_lines': 0,
        'empty_lines': 0,
        'a_instructions': 0,
        'c_instructions': 0,
        'labels': 0,
        'symbols': set()
    }
    
    for line in lines:
        line = line.strip()
        if not line:
            stats['empty_lines'] += 1
        elif line.startswith('//'):
            stats['comment_lines'] += 1
        elif line.startswith('(') and line.endswith(')'):
            stats['labels'] += 1
            stats['code_lines'] += 1
            symbol = line[1:-1]
            stats['symbols'].add(symbol)
        elif line.startswith('@'):
            stats['a_instructions'] += 1
            stats['code_lines'] += 1
            symbol = line[1:]
            if not symbol.isdigit():
                stats['symbols'].add(symbol)
        elif '=' in line or ';' in line:
            stats['c_instructions'] += 1
            stats['code_lines'] += 1
        else:
            stats['code_lines'] += 1
    
    return stats

def main():
    st.set_page_config(
        page_title="Hack Assembler",
        page_icon="⚙️",
        layout="wide"
    )
    
    st.title("⚙️ Hack Assembler")
    st.markdown("Assemble Hack Assembly code to Hack Machine Language")
    
    # Sidebar for examples and info
    with st.sidebar:
        st.header("📚 Examples")
        
        if st.button("Simple A-instruction"):
            st.session_state.asm_code = """// Simple A-instruction example
@16
D=A
@17
M=D"""
        
        if st.button("Basic Program"):
            st.session_state.asm_code = """// Basic program: Add two numbers
@2      // Load constant 2
D=A     // D = 2
@3      // Load constant 3  
D=D+A   // D = 2 + 3
@sum    // Load address of sum
M=D     // Store result in sum
(END)   // Infinite loop
@END
0;JMP"""
        
        if st.button("Loop Example"):
            st.session_state.asm_code = """// Loop example: Count from 1 to 10
@i      // Initialize counter
M=1     // i = 1
@sum    // Initialize sum
M=0     // sum = 0
(LOOP)  // Loop label
@i      // Load i
D=M     // D = i
@sum    // Load sum
M=M+D   // sum = sum + i
@i      // Load i
M=M+1   // i = i + 1
D=M     // D = i
@10     // Load constant 10
D=D-A   // D = i - 10
@LOOP   // If i <= 10, continue loop
D;JLE
(END)   // End program
@END
0;JMP"""
        
        if st.button("Screen Pixel Example"):
            st.session_state.asm_code = """// Set a pixel on the screen
@SCREEN // Screen memory map starts at 16384
D=A     // D = screen base address
@pixel  // Address to store pixel location
M=D     // pixel = screen base
@pixel  // Load pixel address
A=M     // A = pixel address  
M=-1    // Set all 16 pixels in word to 1"""
        
        st.header("ℹ️ About")
        st.markdown("""
        This assembler converts Hack Assembly code to 16-bit machine language.
        
        **Supported Instructions:**
        - **A-instructions**: `@value` or `@symbol`
        - **C-instructions**: `dest=comp;jump`
        - **Labels**: `(LABEL)`
        - **Comments**: `// comment`
        
        **Predefined Symbols:**
        - Registers: R0-R15, SP, LCL, ARG, THIS, THAT
        - I/O: SCREEN (16384), KBD (24576)
        - Virtual registers: Used for additional storage
        """)
    
    # Main content area
    col1, col2 = st.columns([1, 1])
    
    with col1:
        st.header("📥 Input Assembly Code")
        
        # Initialize session state
        if 'asm_code' not in st.session_state:
            st.session_state.asm_code = """// Enter your assembly code here
@2
D=A
@3
D=D+A
@sum
M=D"""
        
        asm_input = st.text_area(
            "Assembly Code:",
            value=st.session_state.asm_code,
            height=400,
            placeholder="Enter your Hack assembly code here..."
        )
        
        # Update session state
        st.session_state.asm_code = asm_input
        
        # Code analysis
        if asm_input.strip():
            with st.expander("📊 Code Analysis", expanded=False):
                stats = analyze_assembly_code(asm_input)
                col1_1, col1_2 = st.columns(2)
                
                with col1_1:
                    st.metric("Total Lines", stats['total_lines'])
                    st.metric("Code Lines", stats['code_lines'])
                    st.metric("A-instructions", stats['a_instructions'])
                
                with col1_2:
                    st.metric("Comment Lines", stats['comment_lines'])
                    st.metric("C-instructions", stats['c_instructions'])
                    st.metric("Labels", stats['labels'])
                
                if stats['symbols']:
                    st.markdown("**Symbols used:**")
                    st.write(", ".join(sorted(stats['symbols'])))
        
        col1_1, col1_2 = st.columns(2)
        
        with col1_1:
            if st.button("⚙️ Assemble", type="primary"):
                if asm_input.strip():
                    st.session_state.assemble_clicked = True
                else:
                    st.warning("Please enter some assembly code to assemble.")
        
        with col1_2:
            if st.button("🗑️ Clear"):
                st.session_state.asm_code = ""
                st.session_state.assemble_clicked = False
                st.rerun()
    
    with col2:
        st.header("📤 Output Machine Code")
        
        # Add toggle for output format
        output_format = st.radio(
            "Output Format:",
            ["Clean (Binary only)", "Detailed (With annotations)"],
            horizontal=True
        )
        
        if st.button("⚙️ Assemble", key="assemble_right") or st.session_state.get('assemble_clicked', False):
            if asm_input.strip():
                # Reset the flag
                st.session_state.assemble_clicked = False
                
                # Show compilation status
                with st.spinner("Compiling assembler..."):
                    if not compile_assembler():
                        st.error("Failed to compile assembler. Please check the C++ code.")
                        return
                
                # Show assembly status
                with st.spinner("Assembling code..."):
                    machine_output = assemble_hack_code(asm_input)
                
                if machine_output:
                    st.success("✅ Assembly completed successfully!")
                    
                    # Display formatted output based on user choice
                    if output_format == "Clean (Binary only)":
                        st.markdown("### Generated Machine Code:")
                        st.code(format_machine_code(machine_output), language="text")
                    else:
                        st.markdown("### Generated Machine Code (Detailed):")
                        st.code(format_machine_code_detailed(machine_output), language="text")
                    
                    # Download button
                    st.download_button(
                        label="📥 Download Machine Code File",
                        data=machine_output,
                        file_name="output.hack",
                        mime="text/plain"
                    )
                    
                    # Statistics
                    asm_lines = len([line for line in asm_input.strip().split('\n') if line.strip() and not line.strip().startswith('//') and not (line.strip().startswith('(') and line.strip().endswith(')'))])
                    machine_lines = len([line for line in machine_output.strip().split('\n') if line.strip()])
                    
                    st.info(f"📊 **Statistics:** {asm_lines} assembly instructions → {machine_lines} machine instructions")
                else:
                    st.error("❌ Assembly failed. Please check your assembly code syntax.")
            else:
                st.warning("Please enter some assembly code to assemble.")
        else:
            st.info("👈 Enter assembly code and click 'Assemble' to see the machine code output.")
    
    # Footer
    st.markdown("---")
    st.markdown(
        "**Hack Assembler** - Part of the Nand2Tetris project. "
        "Converts Hack Assembly code to 16-bit machine language."
    )

if __name__ == "__main__":
    main()
