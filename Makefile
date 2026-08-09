TARGET_WIFI = App_WiFi
TARGET_SPY = App_Spy

DEVICE ?= PC

START_PATH ?= "/"
RES_PATH ?= "./res"

ifeq ($(DEVICE),PC)
	CC = g++
	SDL2_CONFIG = sdl2-config
	START_PATH = $(PWD)
else
	CC = $(CXX)
	SDL2_CONFIG = /usr/bin/sdl2-config
endif

COMPILER_FLAGS = $(shell $(SDL2_CONFIG) --cflags) -Wall -pedantic -DDEVICE_$(DEVICE) -DSTART_PATH=\"$(START_PATH)\" -DRES_PATH=\"$(RES_PATH)\"
LINKER_FLAGS = $(shell $(SDL2_CONFIG) --libs) -pthread

all : $(TARGET_WIFI) $(TARGET_SPY)

$(TARGET_WIFI): src/WiFi_Tester.o
	$(CC) $< -o $@ $(LINKER_FLAGS)

$(TARGET_SPY): src/Spy_Tester.o
	$(CC) $< -o $@ $(LINKER_FLAGS)

src/%.o: src/%.cpp
	$(CC) -c $< -o $@ $(COMPILER_FLAGS)

clean :
	rm -f src/*.o $(TARGET_WIFI) $(TARGET_SPY)
