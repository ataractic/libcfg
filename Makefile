CFLAGS = -Wall -Wconversion -Wextra -fPIC

# Find all .c files excluding those in n1.ko directory
SRC_FILES := $(shell find . -name '*.c' ! -path './tests/*' ! -path './fuzz/*')

shared:
	clang -std=gnu2x -shared -o libcfg.so $(SRC_FILES) $(CFLAGS)

clean:
	rm -rf *.o n1