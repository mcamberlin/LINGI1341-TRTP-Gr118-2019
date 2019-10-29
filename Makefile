# See gcc/clang manual to understand all flags
CFLAGS += -std=c99 # Define which version of the C standard to use
CFLAGS += -Wall # Enable the 'all' set of warnings
CFLAGS += -Werror # Treat all warnings as error
CFLAGS += -Wshadow # Warn when shadowing variables
CFLAGS += -Wextra # Enable additional warnings
CFLAGS += -O2 -D_FORTIFY_SOURCE=2 # Add canary code, i.e. detect buffer overflows
CFLAGS += -fstack-protector-all # Add canary code to detect stack smashing
CFLAGS += -D_POSIX_C_SOURCE=201112L -D_XOPEN_SOURCE # feature_test_macros for getpot and getaddrinfo

# We have no libraries to link against except libc, but we want to keep
# the symbols for debugging
LDFLAGS= -lz

# Default target
all: receiver

receiver : 
	gcc -o receiver src/receiver.c src/LinkedList.c src/packet_implem.c src/read_write_loop.c src/socket.c src/ConnexionList.c -lz $(CFLAGS)

tests: receiver
	./receiver -o "fichier_%02d.dat" :: 12345


clean:
	@rm -f receiver
