** Tests for bytetide project **

# Author
Zeyu (Jack) Chu

# Date
Sun May 12 21:14:39 AEST 2024

# AI used (yes/no)
yes. perplexity AI pro

# AI link
https://www.perplexity.ai/

# Test 1 Description − Invalid bpkg files
Testing that error is returned when parsing invalid bpkg files. Description of testcases below
1. Invalid identity: not all characters are not hexadecimal
1. Missing filename: filename line not present
1. Mismatched nhash: the amount of hashes given is not the same as the amount of hashes
1. Invalid nhash: nhash does not follow 2^(h-1)-1
1. Mismatched nchunk: the amount of hashes given is not the same as the amount of hashes
1. Invalid nchunk: nchunk does not follow 2^(h-1)-1
1. Invalid hash: hash is too short
1. Invalid chunk: chunk too short
1. Invalid chunk size: chunk size incorrect

The expected output should be "Unable to load pkg and tree" for all testcases. If output is correct, nothing will be printed, otherwise the the diff will be printed to the terminal.

# Test 2 Description − Testing merkle tree is parsed correct and merkle tree traversal operations

There are two parts being tested:

The parsing is tested with run_tests_parse.sh. Here we tests that all hashes and chunks are parsed correctly from bpkg file. We will be using the four different generated files in datafiles directory to test these functions. See create_data.sh for more details regarding generation of datafiles. If the corresponding hashes or chunks are incorrect an error message will print and the difference will be printed to the terminal.

The second part we test we are able to obtain all chunks from any hash in the merkletree. Test case 1 and 3 tests root hash and test case 2 and 4 test child hashes. We will be using the same data generated in test 2. You can test this by running run_tests_traversals.sh

# AI Prompt 1
https://www.perplexity.ai/search/A-bpkg-file-YC33HJreQmehEYWt8fUr.g  
A bpkg file has the following structure: ... Can you create some invalid bpkg files for testing purposes.  
Some follow up question were asked that includes an example of a bpkg file format and specific instructions for the following testcase  
1. Mismatched nchunk: the amount of hashes given is not the same as the amount of hashes
1. Invalid nchunk: nchunk does not follow 2^(h-1)-1
1. Invalid hash: hash is too short
1. Invalid chunk: chunk too short
1. Invalid chunk size: chunk size incorrect

(To see full prompt visit link)

# AI Generated 1
AI generated some examples of invalid formats, these examples have been converted into text files with modifications(e.g. removing some spaces and adding some tabs and changing some numbers around).  
All test cases are present in bpkg files at ./tests/invalid_bpkg

# AI Prompt 2
https://www.perplexity.ai/search/Can-you-write-7BgA3m3hSeCiaqfiv1sCdg  
Can you write a bash script to create 5 different bpkg and dat files using a program with following usage:  
Usage:  
pkgmake <file>  
--chunksz <chunk size>  
--nchunks <number of chunks>  
--output <filename>  
Example: pkgmake somedatafile.dat --nchunks 32 --output somedatafile.bpkg  
Make sure the files created have some variance as they will be used for testing purposes

The prompt had further conversation, see full conversation in link

# AI Generated 2
See bash script in ./tests/datafiles/create_data.sh

# AI Prompt 3
https://www.perplexity.ai/search/Write-a-bash-aYxV6ZiNR62KRSBzhwRlbw  
Here is a bash script that runs a binary named package_main on files named sample_{i}, where i ranges from 1 to 4. The script then compares the output of each run against a corresponding file named sample{i}_chunk.out.
# AI Generated 3
Bash code has been modified slightly and present in ./tests/merkletree_parse/run_tests.sh and ./tests/merkletree_traversals/run_tests.sh

# Instructions to run testing script
Go into the folder you wish to run tests. To run all tests in a folder, run below
```bash
./run_tests.sh
```