

# Default compiler and compiler flags
CC=gcc

# Default flags for all compilers
O_FLAGS=-Wall -Wextra -Werror -pedantic -O2
# Debugging flags
CC_FLAGS=$(O_FLAGS) -std=c99


# Binaries, object files, libraries and stuff
BINS=timeout


# Default generic instructions
default:	all
all:	$(BINS)
clean:	
	rm -f *.o
	
timeout:	timeout.c
	$(CC) $(CC_FLAGS) -o $@ $< -D_POSIX_SOURCE

install:	timeout
	install timeout /usr/local/bin
