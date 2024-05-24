#!/bin/bash

# check for btide
if [ ! -f "./btide" ]; then
    echo "Btide binary file not found"
    exit 1
fi

# Compile the C program
gcc network_test.c -o network_test
if [ $? -ne 0 ]; then
    echo "Compilation failed."
    exit 1
fi

# Run the binary with different configuration files
./network_test config1.cfg > output1.txt &
./network_test config2.cfg a > output2.txt &
./network_test config3.cfg a > output3.txt &
./network_test config4.cfg a > output4.txt &

wait

# Compare the outputs
echo "Main peer diff... (should print nothing)"
diff output1.txt main.out
echo "Side peer diff... (should print nothing)"
diff output2.txt side.out
diff output3.txt side.out
diff output4.txt side.out

# remove output files
rm output1.txt output2.txt output3.txt output4.txt