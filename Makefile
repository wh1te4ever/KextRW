TARGET := KextRW
SRC    := src
TESTS  := tests
LIBDIR := lib
BUILD  := build
INCLUDE_DIR := $(BUILD)/include
LIB_OUT := $(BUILD)/lib
BIN_OUT := $(BUILD)/bin

# Don't use ?= with $(shell ...)
ifndef CXX_FLAGS
CXX_FLAGS := --std=gnu++17 -Wall -O3 -nostdinc -nostdlib -mkernel -DKERNEL -isystem $(shell xcrun --show-sdk-path)/System/Library/Frameworks/Kernel.framework/Headers -Wl,-kext -lcc_kext $(CXXFLAGS)
endif

.PHONY: all clean lib build tests install

all: $(TARGET).kext/Contents/_CodeSignature/CodeResources lib tests

$(TARGET).kext/Contents/MacOS/$(TARGET): $(SRC)/*.S $(SRC)/*.cpp $(SRC)/*.h | $(TARGET).kext/Contents/MacOS
	$(CXX) -arch arm64e -o $@ $(SRC)/*.S $(SRC)/*.cpp $(CXX_FLAGS)

$(TARGET).kext/Contents/Info.plist: misc/Info.plist | $(TARGET).kext/Contents
	cp -f $^ $@

$(TARGET).kext/Contents/_CodeSignature/CodeResources: $(TARGET).kext/Contents/MacOS/$(TARGET) $(TARGET).kext/Contents/Info.plist
	codesign -s - -f $(TARGET).kext

$(TARGET).kext/Contents $(TARGET).kext/Contents/MacOS:
	mkdir -p $@

build:
	mkdir -p $(INCLUDE_DIR) $(LIB_OUT) $(BIN_OUT)

lib: build
	$(MAKE) -C $(LIBDIR) BUILD_DIR=$(BUILD)
	cp -r $(LIBDIR)/*.h $(INCLUDE_DIR)
	mv $(LIBDIR)/*.a $(LIB_OUT)

tests: build
	$(MAKE) -C $(TESTS) BUILD_DIR=../$(BUILD)

install: all
	sudo cp -r $(TARGET).kext /Library/Extensions

clean:
	sudo rm -rf $(TARGET).kext $(BUILD)
	$(MAKE) -C $(TESTS) clean
	$(MAKE) -C $(LIBDIR) clean
