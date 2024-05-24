# File structure
The header files are in the include folder and sources files in src folder. The test folders are named network_tests and pkgmain_tests. There are two parts of the program that are seperated.

The first part relates to processing the raw data. This includes parsing of a bpkg file which contains metadata about the data and checking the hash of the file to ensure its integrity. The files related to hashing are in the crypt folder. In the merkletree folder is the logic for merkletree which allows us to efficiently decipher which parts of the file is missing or incorrect. the pkgchk.c file contains the logic that ties everything together and contains an api for the main btide program to obtain information about the data. The two parts are seperated for modularity and easier testing and debugging.

The second part relates to all networking logic. The sources files are stored in src/btide folder. Description of each file follows

### config.c
This files contains logic in parsing the config file.

### network.c
This file contains logic relating to the protocol by which the application communicates with each other. This include logic for the main server threads that accept connections and the logic for each peer thread that maintains connections between between sockets.

### package.c
All packages are stored in a linked list. All operations relating to packages and the linked list are here.

### peer.c
Each connection with a peer will have a peer object/struct related to it. They will all be stored in a linked list. All operations relating to peer are stored here

### packet.c
Logic pertaining to defining and creating the packets that are sent over the network are here.

The logic connecting the CLI to the networking threads are stored in btide.c. CLI commands are parsed and executed here.

# Tests
Tests for each part of the project is explained further in the pkgchk_test.md and network_test.md files and their respective folders

# How to compile and run program
To compile the btide program, run this in bash in the main directory containing the makefile
`make btide`
To run btide, you would need a config file that specifies the directory of the data files, the max amount of peers and the port. a sample config file is given to you in the main folder. Run the followling replacing the path inside <> with the path to your config file.
`btide <path/to/sample_config.cfg>`