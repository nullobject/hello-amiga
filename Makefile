.PHONY: all clean
.SECONDARY:

CONFIG = +kick13
STARTUP_FILE = uae/dh0/s/startup-sequence

EXAMPLES = $(wildcard examples/*.c)
EXES = $(addprefix build/,$(notdir $(EXAMPLES:.c=)))
C_SOURCES = $(wildcard src/*.c)
OBJECTS = $(addprefix build/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
S_SOURCES = $(wildcard src/*.s)
OBJECTS += $(addprefix build/,$(notdir $(S_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(S_SOURCES)))

all: $(OBJECTS) $(EXES)

clean:
	rm -rf build

example%: build/example% build/8c-tileset.ts build/8c-level.lvl build/32c-tileset.ts build/32c-level.lvl build/rodland_bobs.ts
	mkdir -p build/s
	echo sys:$@ > build/s/startup-sequence
	fs-uae --hard_drive_0=build --automatic_input_grab=0

build/example%: examples/example%.c $(OBJECTS) | build
	vc $(CONFIG) -lamiga -lauto -g -I$(NDK_INC) -Isrc -o $@ $^

build/%.o: %.c | build
	vc $(CONFIG) -c99 -g -c -I$(NDK_INC) -o $@ $<

build/%.o: %.s | build
	vc $(CONFIG) -g -c -o $@ $<

build/8c-tileset.ts build/8c-level.lvl &: assets/8c-tiles.png assets/8c-tileset.json assets/8c-map.json | build
	ratr0-converttiled assets/8c-tileset.json assets/8c-map.json build/8c-tileset.ts build/8c-level.lvl

build/32c-tileset.ts build/32c-level.lvl &: assets/32c-tiles.png assets/32c-tileset.json assets/32c-map.json | build
	ratr0-converttiled assets/32c-tileset.json assets/32c-map.json build/32c-tileset.ts build/32c-level.lvl

build/rodland_bobs.ts: assets/rodland_bobs.png | build
	ratr0-maketiles -ts 44x31 -cm assets/rodland_bobs.png build/rodland_bobs.ts

build:
	mkdir -p $@
