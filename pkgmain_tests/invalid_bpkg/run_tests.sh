#!/bin/bash

# Define the path to the binary
binaryPath="../bin/pkgmain"

# Check if the binary exists
if [ ! -f "$binaryPath" ]; then
    echo "Binary file not found at $binaryPath"
    exit 1
fi

# Loop through all .bpkg files in the current directory
for file in test*.bpkg; do
    # Print the name of the file being run
    echo "Running test on: $file"
    # Execute the binary with the name of the current file
    "$binaryPath" "$file" -all_hashes | diff -w - expected.out
done