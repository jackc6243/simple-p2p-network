CC=gcc
CFLAGS=-Wall -std=c2x -g
DEBUG = -fsanitize=address -g -DDEBUG
LDFLAGS=-lm -lpthread
INCLUDE=-Iinclude
OBJS = pkgchk.o merkletree.o sha256.o
BTIDE_OBJS = config.o peer.o network.o packet.o package.o
.PHONY: clean

# Required for Part 1 - Make sure it outputs a .o file
# to either objs/ or ./
# In your directory
sha256.o: src/crypt/sha256.c include/crypt/sha256.h
	$(CC) -c $< $(INCLUDE) $(CFLAGS) $(LDFLAGS)

merkletree.o: src/tree/merkletree.c include/tree/merkletree.h
	$(CC) -c $< $(INCLUDE) $(CFLAGS) $(LDFLAGS)

pkgchk.o: src/chk/pkgchk.c include/chk/pkgchk.h include/crypt/sha256.h include/tree/merkletree.h
	$(CC) -c $< $(INCLUDE) $(CFLAGS) $(LDFLAGS)

pkgmain.o: src/pkgmain.c include/chk/pkgchk.h include/tree/merkletree.h include/crypt/sha256.h
	$(CC) -c $< $(INCLUDE) $(CFLAGS) $(LDFLAGS)

pkgmain: pkgmain.o $(OBJS)
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o $@

debug: CFLAGS += $(DEBUG)
debug: pkgmain

# Required for Part 2 - Make sure it outputs `btide` file
# in your directory ./

config.o: src/btide/config.c include/chk/pkgchk.h  include/net/config.h
	$(CC) -c $< $(INCLUDE) $(CFLAGS) $(LDFLAGS)

network.o: src/btide/network.c include/net/packet.h include/net/network.h
	$(CC) -c $< $(INCLUDE) $(CFLAGS) $(LDFLAGS)

peer.o: src/btide/peer.c include/net/peer.h include/net/packet.h
	$(CC) -c $< $(INCLUDE) $(CFLAGS) $(LDFLAGS)

package.o: src/btide/package.c include/net/package.h
	$(CC) -c $< $(INCLUDE) $(CFLAGS) $(LDFLAGS)

packet.o: src/btide/packet.c include/net/packet.h include/tree/merkletree.h include/chk/pkgchk.h include/crypt/sha256.h
	$(CC) -c $< $(INCLUDE) $(CFLAGS) $(LDFLAGS)

btide.o: src/btide.c include/chk/pkgchk.h include/net/packet.h include/net/config.h include/net/network.h
	$(CC) -c $< $(INCLUDE) $(CFLAGS) $(LDFLAGS)

btide: $(OBJS) $(BTIDE_OBJS) btide.o
	$(CC) $^ $(INCLUDE) $(CFLAGS) $(LDFLAGS) -o $@


# Alter your build for p1 tests to build unit-tests for your
# merkle tree, use pkgchk to help with what to test for
# as well as some basic functionality
p1tests:
	bash p1test.sh

# Alter your build for p2 tests to build IO tests
# for your btide client, construct .in/.out files
# and construct a script to help test your client
# You can opt to constructing a program to
# be the tests instead, however please document
# your testing methods
p2tests:
	bash p2test.sh

clean:
	rm -f *.o pkgmain btide