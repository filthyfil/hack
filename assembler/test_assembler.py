import os
import subprocess

# Ensure the working directory is the same as the script's directory.
os.chdir(os.path.dirname(os.path.abspath(__file__)))

def run_assembler(asm_file):
    """
    Run the hackassembler executable and pass the input file name as user input.
    """
    process = subprocess.Popen(
        ["./hackassembler"],       # Command to run the executable
        stdin=subprocess.PIPE,     # Pass input via stdin
        stdout=subprocess.PIPE,    # Capture stdout
        stderr=subprocess.PIPE,    # Capture stderr
        text=True                  # Use text mode for input/output
    )
    
    # Pass the input file name to the program.
    stdout, stderr = process.communicate(input=asm_file + "\n")
    
    # Check for errors.
    if process.returncode != 0:
        raise RuntimeError(f"Error running hackassembler: {stderr}")

def read_file(file_path):
    """
    Read the contents of a file and return it as a string.
    """
    with open(file_path, "r") as file:
        return file.read()

def test_assembler():
    # Define the input ASM file and expected output file.
    asm_file = "asm_test_input.txt"
    expected_output_file = "binary_test_output.txt"
    actual_output_file = "output.txt"

    # Run the assembler and pass the input file name.
    run_assembler(asm_file)

    # Read the actual and expected output.
    actual_output = read_file(actual_output_file)
    expected_output = read_file(expected_output_file)

    # Compare the output.
    assert actual_output.strip() == expected_output.strip(), "Test failed: Output does not match expected binary output."

    print("All tests passed!")

if __name__ == "__main__":
    test_assembler()
