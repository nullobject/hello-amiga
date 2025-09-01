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

example%: build/example% build/tileset.ts build/level.lvl
	mkdir -p build/s
	echo sys:$@ > build/s/startup-sequence
	fs-uae --hard_drive_0=build --automatic_input_grab=0

build/example%: examples/example%.c $(OBJECTS) | build
	vc $(CONFIG) -lamiga -lauto -g -I$(NDK_INC) -Isrc -o $@ $^

build/%.o: %.c | build
	vc $(CONFIG) -c99 -g -c -I$(NDK_INC) -o $@ $<

build/%.o: %.s | build
	vc $(CONFIG) -g -c -o $@ $<

build/tileset.ts build/level.lvl &: assets/tileset.json assets/map.json | build
	ratr0-converttiled $^ build/tileset.ts build/level.lvl

build:
	mkdir -p $@
