#!/bin/bash

# Define the path to the binary
binaryPath="../bin/pkgmain"

# Check if the binary exists
if [ ! -f "$binaryPath" ]; then
    echo "Binary file not found at $binaryPath"
    exit 1
fi

# Define the arguments given
declare -a input_files=("sample_valid_1.bpkg" "sample_valid_1.bpkg" "sample_valid_2.bpkg" "sample_valid_2.bpkg")
declare -a expected_output_files=("test1_sample1_root" "test2_sample1_child" "test3_sample2_root" "test4_sample2_child")
declare -a hashes=("fdc07cd322fc2533ceaf8a39219c62e5b9ef63f66e4c0bae8bccf455ea5bfee6" "19f0456e2ac567ccb674a4da1d1a579be31557ea9ed75152584986f175785e88" "095d08f9d326668e6d3b085982079d5a75b366d9f1cf80a0740360a0c605aa24" "d53993b39355dc1a0951e79b0be6e6e78a4c9d0041367e8cb47d63e953d6789d")

# Loop through the numbers 1 to 4
for i in {0..3}
do
    input_file=${input_files[$i]}
    expected_output_file=${expected_output_files[$i]}.out
    hash=${hashes[$i]}

    # Run the binary with the input file and capture the output
    $binaryPath $input_file "-hashes_of" ${hashes[$i]} > temp_output.txt

    # Compare the output of the binary with the expected output
    echo "Testing $expected_output_file, differences are..."
    diff -w temp_output.txt $expected_output_file
done

# Clean up the temporary output file
rm temp_output.txt