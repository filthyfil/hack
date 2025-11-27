import streamlit as st
import subprocess
import tempfile
import os
import re

def compile_vm_translator():
    """Compile the VM translator C++ code"""
    try:
        result = subprocess.run(
            ["g++", "-o", "VM/hackVM2", "VM/hackVM2.cpp"],
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

def translate_vm_to_asm(vm_code, filename="input"):
    """Translate VM code to assembly using the compiled translator"""
    try:
        # Create a temporary VM input file (translator produces .asm next to it)
        with tempfile.NamedTemporaryFile(mode='w', suffix='.vm', delete=False) as vm_file:
            vm_file.write(vm_code)
            vm_file_path = vm_file.name

        # Run the translator binary (similar to assembler_app: binary is at ../VM/hackVM2
        # relative to the streamlit_apps working directory)
        result = subprocess.run(
            ["../VM/hackVM2", vm_file_path],
            capture_output=True,
            text=True,
            cwd="."
        )

        # Always remove the temporary VM input file
        try:
            os.unlink(vm_file_path)
        except Exception:
            pass

        if result.returncode != 0:
            st.error(f"Translation failed: {result.stderr}")
            # Remove any produced .asm file if present
            asm_path = vm_file_path[:-3] + ".asm"
            try:
                if os.path.exists(asm_path):
                    os.unlink(asm_path)
            except Exception:
                pass
            return None

        # Read the .asm file the translator produces next to the input .vm
        asm_path = vm_file_path[:-3] + ".asm"
        if not os.path.exists(asm_path):
            st.error("Translation did not produce an output .asm file.")
            return None

        with open(asm_path, 'r') as f:
            assembly_output = f.read()

        # Clean up produced asm
        try:
            os.unlink(asm_path)
        except Exception:
            pass

        return assembly_output

    except Exception as e:
        st.error(f"Translation error: {str(e)}")
        return None

def format_assembly(assembly_code):
    """Format assembly code for better display"""
    lines = assembly_code.strip().split('\n')
    formatted_lines = []
    
    for i, line in enumerate(lines, 1):
        if line.strip():
            # Simple line numbering without markdown clutter
            formatted_lines.append(f"{i:3d}: {line}")
    
    return '\n'.join(formatted_lines)

def format_assembly_detailed(assembly_code):
    """Format assembly code with instruction type annotations"""
    lines = assembly_code.strip().split('\n')
    formatted_lines = []
    
    for i, line in enumerate(lines, 1):
        if line.strip():
            # Add instruction type annotations
            if line.startswith('('):
                formatted_lines.append(f"{i:3d}: {line}  // Label")
            elif line.startswith('@'):
                formatted_lines.append(f"{i:3d}: {line}  // A-instruction")
            elif ';' in line:
                formatted_lines.append(f"{i:3d}: {line}  // Jump")
            elif '=' in line:
                formatted_lines.append(f"{i:3d}: {line}  // C-instruction")
            else:
                formatted_lines.append(f"{i:3d}: {line}")
    
    return '\n'.join(formatted_lines)

def main():
    st.set_page_config(
        page_title="Hack VM Translator",
        page_icon="⚙️",
        layout="wide"
    )
    
    st.title("⚙️ Hack VM Translator")
    st.markdown("Translate Hack Virtual Machine code to Hack Assembly code")
    
    # Sidebar for examples and info
    with st.sidebar:
        st.header("📚 Examples")
        
        if st.button("Basic Arithmetic"):
            st.session_state.vm_code = """push constant 7
push constant 8
add
push constant 15
sub
neg"""
        
        if st.button("Comparison Operations"):
            st.session_state.vm_code = """push constant 10
push constant 5
gt
push constant 5
push constant 10
lt
push constant 7
push constant 7
eq"""
        
        if st.button("Function Call"):
            st.session_state.vm_code = """function mult 2
    push argument 0
    push argument 1
    call Math.multiply 2
    return

call mult 2"""
        
        st.header("ℹ️ About")
        st.markdown("""
        This translator converts Hack VM code to Hack Assembly code.
        
        **Supported Commands:**
        - Stack arithmetic: `add`, `sub`, `neg`, `eq`, `gt`, `lt`, `and`, `or`, `not`
        - Memory access: `push`/`pop` for all segments
        - Program flow: `label`, `goto`, `if-goto`
        - Function calls: `function`, `call`, `return`
        """)
    
    # Main content area
    col1, col2 = st.columns([1, 1])
    
    with col1:
        st.header("📥 Input VM Code")
        
        # Initialize session state
        if 'vm_code' not in st.session_state:
            st.session_state.vm_code = """// Enter your VM code here
push constant 17
push constant 18
add
push constant 19
sub"""
        
        vm_input = st.text_area(
            "VM Code:",
            value=st.session_state.vm_code,
            height=400,
            placeholder="Enter your VM code here..."
        )
        
        # Update session state
        st.session_state.vm_code = vm_input
        
        col1_1, col1_2 = st.columns(2)
        
        with col1_1:
            if st.button("🔄 Translate", type="primary"):
                if vm_input.strip():
                    st.session_state.translate_clicked = True
                else:
                    st.warning("Please enter some VM code to translate.")
        
        with col1_2:
            if st.button("🗑️ Clear"):
                st.session_state.vm_code = ""
                st.session_state.translate_clicked = False
                st.rerun()
    
    with col2:
        st.header("📤 Output Assembly")
        
        # Add toggle for output format
        output_format = st.radio(
            "Output Format:",
            ["Clean (Line numbers only)", "Detailed (With instruction types)"],
            horizontal=True
        )
        
        if st.button("🔄 Translate", key="translate_right") or st.session_state.get('translate_clicked', False):
            if vm_input.strip():
                # Reset the flag
                st.session_state.translate_clicked = False
                
                # Show compilation status
                with st.spinner("Compiling VM translator..."):
                    if not compile_vm_translator():
                        st.error("Failed to compile VM translator. Please check the C++ code.")
                        return
                
                # Show translation status
                with st.spinner("Translating VM to Assembly..."):
                    assembly_output = translate_vm_to_asm(vm_input)
                
                if assembly_output:
                    st.success("✅ Translation completed successfully!")
                    
                    # Display formatted output based on user choice
                    if output_format == "Clean (Line numbers only)":
                        st.markdown("### Generated Assembly Code:")
                        st.code(format_assembly(assembly_output), language="assembly")
                    else:
                        st.markdown("### Generated Assembly Code (Detailed):")
                        st.code(format_assembly_detailed(assembly_output), language="assembly")
                    
                    # Download button
                    st.download_button(
                        label="📥 Download Assembly File",
                        data=assembly_output,
                        file_name="output.asm",
                        mime="text/plain"
                    )
                    
                    # Statistics
                    vm_lines = len([line for line in vm_input.strip().split('\n') if line.strip() and not line.strip().startswith('//')])
                    asm_lines = len([line for line in assembly_output.strip().split('\n') if line.strip()])
                    
                    st.info(f"📊 **Statistics:** {vm_lines} VM instructions → {asm_lines} assembly instructions")
                else:
                    st.error("❌ Translation failed. Please check your VM code syntax.")
            else:
                st.warning("Please enter some VM code to translate.")
        else:
            st.info("👈 Enter VM code and click 'Translate' to see the assembly output.")
    
    # Footer
    st.markdown("---")
    st.markdown(
        "**Hack VM Translator** - Part of the Nand2Tetris project. "
        "Translates Hack Virtual Machine code to Hack Assembly code."
    )

if __name__ == "__main__":
    main()
