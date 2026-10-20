CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror
SDL_CFLAGS := $(shell pkg-config --cflags sdl2)
SDL_LIBS := $(shell pkg-config --libs sdl2)
ROOT := ..
BUILD := build
CORE := $(BUILD)/flight_math.o $(BUILD)/flight_input.o $(BUILD)/movement.o $(BUILD)/ship_energy.o $(BUILD)/map.o
.PHONY: all clean test
all: $(BUILD)/sniper87-linux
$(BUILD):
	mkdir -p $@
$(BUILD)/flight_math.o: $(ROOT)/src/flight_math.c $(ROOT)/src/flight_math.h | $(BUILD)
	$(CC) $(CFLAGS) -Isrc -I$(ROOT)/src -fPIC -c $< -o $@
$(BUILD)/flight_input.o: $(ROOT)/src/flight_input.c $(ROOT)/src/flight_input.h | $(BUILD)
	$(CC) $(CFLAGS) -I$(ROOT)/src -c $< -o $@
$(BUILD)/movement.o: $(ROOT)/src/movement.c $(ROOT)/src/movement.h | $(BUILD)
	$(CC) $(CFLAGS) -I$(ROOT)/src -c $< -o $@
$(BUILD)/ship_energy.o: $(ROOT)/src/ship_energy.c $(ROOT)/src/ship_energy.h | $(BUILD)
	$(CC) $(CFLAGS) -I$(ROOT)/src -c $< -o $@
$(BUILD)/map.o: $(ROOT)/src/map.c $(ROOT)/src/map.h | $(BUILD)
	$(CC) $(CFLAGS) -I$(ROOT)/src -c $< -o $@
$(BUILD)/sniper87-linux: main.c $(CORE) | $(BUILD)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -I$(ROOT)/src $< $(CORE) $(SDL_LIBS) -lm -o $@
test: $(BUILD)/sniper87-linux
	SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy $(BUILD)/sniper87-linux --frames 120
clean:
	rm -rf $(BUILD)
