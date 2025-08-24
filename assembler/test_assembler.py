import subprocess

def run_assembler(asm_file):
    """
    Run the hackassembler executable and pass the input file name as user input.
    """
    # Compile the C++ code
    compile_process = subprocess.run(
        ["g++", "-o", "assembler/hackassembler", "assembler/hackassembler.cpp"],
        capture_output=True,
        text=True
    )
    if compile_process.returncode != 0:
        raise RuntimeError(f"Error compiling Assembler: {compile_process.stderr}")

    # Run the compiled executable
    process = subprocess.Popen(
        ["./assembler/checkassembler", asm_file, "assembler/output.txt"],
        stdout=subprocess.PIPE,    # Capture stdout
        stderr=subprocess.PIPE,    # Capture stderr
        text=True                  # Use text mode for input/output
    )
    
    # Wait for the process to complete
    stdout, stderr = process.communicate()
    
    # Check for errors
    if process.returncode != 0:
        raise RuntimeError(f"Error running Assembler: {stderr}")

def read_file(file_path):
    """
    Read the contents of a file and return it as a string.
    """
    with open(file_path, "r") as file:
        return file.read()

def test_assembler():
    # Define the input ASM file and expected output file
    asm_file = "assembler/asm_test_input.txt"
    expected_output_file = "assembler/binary_test_output.txt"
    actual_output_file = "assembler/output.txt"

    # Run the assembler and pass the input file name
    run_assembler(asm_file)

    # Read the actual and expected output
    actual_output = read_file(actual_output_file)
    expected_output = read_file(expected_output_file)

    # Compare the output
    assert actual_output.strip() == expected_output.strip(), "Test failed: Output does not match expected binary code."

    print("All tests passed!")

if __name__ == "__main__":
    test_assembler()