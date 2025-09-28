TESTS = $(wildcard *.c)
CC = gcc
CFLAGS = -Wall -Wextra -I../build/include
LDFLAGS = -L../build/lib -lkextrw
FRAMEWORKS = -framework IOKit -framework CoreFoundation
BUILD_DIR = ../build

all:
	mkdir -p $(BUILD_DIR)/bin
	@for test in $(TESTS); do \
		$(CC) $(CFLAGS) $(LDFLAGS) $(FRAMEWORKS) $$test -o $(BUILD_DIR)/bin/$${test%.c}; \
		ldid -Sentitlements.plist $(BUILD_DIR)/bin/$${test%.c}; \
	done
	sudo chown -R 0:0 $(BUILD_DIR)/../KextRW.kext
	sudo chown -R 0:0 $(BUILD_DIR)/../KextRW.kext/*
	sudo chown -R 0:0 $(BUILD_DIR)/../KextRW.kext/*/*
	sudo chown -R 0:0 $(BUILD_DIR)/../KextRW.kext/*/*/*

clean:
	rm -rf $(BUILD_DIR)/bin
