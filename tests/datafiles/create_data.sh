#!/bin/bash

# Define base file name
base_name="sample_valid"

# Define number of chunks as powers of 2
declare -a num_chunks=("8" "16" "32" "64" "128")

# Loop to create 8 different files
for i in {1..5}
do
    # Generate .dat file name
    dat_file="${base_name}_${i}.dat"
    
    # Generate .bpkg file name
    bpkg_file="${base_name}_${i}.bpkg"
    
    # Create a .dat file with random data
    # Using a fixed block size (bs) of 4096 bytes and count as num_chunks
    dd if=/dev/urandom of=$dat_file bs=4096 count=${num_chunks[$i]} 2>/dev/null
    
    # Use pkgmake to create .bpkg file from .dat file
    ./pkgmake $dat_file --nchunks ${num_chunks[$i]} --output $bpkg_file
    
    # Output the creation status
    echo "Created $bpkg_file with ${num_chunks[$i]} chunks of 4096 bytes each."
done

echo "All files created successfully."