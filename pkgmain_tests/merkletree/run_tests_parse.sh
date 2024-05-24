#!/bin/bash

# Define the path to the binary
binaryPath="../bin/pkgmain"

# Check if the binary exists
if [ ! -f "$binaryPath" ]; then
    echo "Binary file not found at $binaryPath"
    exit 1
fi

# Loop through the numbers 1 to 4
for i in {1..4}
do
    # Define the input and output file names
    input_file="sample_valid_${i}.bpkg"
    expected_output_file="sample${i}_chunk.out"

    # Run the binary with the input file and capture the output
    "$binaryPath" "$input_file" "-chunk_check" > temp_output.txt

    # Compare the output of the binary with the expected output
    echo "Testing correct chunk parsing, differences are..."
    diff -w temp_output.txt "$expected_output_file"

    # Change the output file names to test hashes instead now
    expected_output_file="sample${i}_hash.out"

    # Run the binary with the input file and capture the output
    "$binaryPath" "$input_file" "-all_hashes" > temp_output.txt

    # Compare the output of the binary with the expected output
    echo "Testing correct hash parsing, differences are..."
    diff -w temp_output.txt "$expected_output_file"
done

# Clean up the temporary output file
rm temp_output.txt