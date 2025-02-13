# test_hackvm.py
import subprocess

def run_vm_translator(vm_file):
    """
    Run the hackVM executable and pass the input file name as user input.
    """
    # Use subprocess to run the hackvm executable and pass the input file name
    process = subprocess.Popen(
        ["./hackVM"],              # Command to run the executable
        stdin=subprocess.PIPE,     # Pass input via stdin
        stdout=subprocess.PIPE,    # Capture stdout
        stderr=subprocess.PIPE,    # Capture stderr
        text=True                  # Use text mode for input/output
    )
    
    # Pass the input file name to the program
    stdout, stderr = process.communicate(input=vm_file + "\n")
    
    # Check for errors
    if process.returncode != 0:
        raise RuntimeError(f"Error running hackvm: {stderr}")

def read_file(file_path):
    """
    Read the contents of a file and return it as a string.
    """
    with open(file_path, "r") as file:
        return file.read()

def test_vm_translator():
    # Define the input VM file and expected output file
    vm_file = "VM_test_commands.txt"
    expected_output_file = "asm_test_commands.txt"
    actual_output_file = "output.txt"

    # Run the VM translator and pass the input file name
    run_vm_translator(vm_file)

    # Read the actual and expected output
    actual_output = read_file(actual_output_file)
    expected_output = read_file(expected_output_file)

    # Compare the output
    assert actual_output.strip() == expected_output.strip(), "Test failed: Output does not match expected assembly code."

    print("All tests passed!")

if __name__ == "__main__":
    test_vm_translator()