.PHONY: all clean
.SECONDARY:

ASSETS_DIR = assets
BUILD_DIR = build
EXAMPLES_DIR = examples
TARGET_DIR = uae/dh0

CONFIG = +kick13
STARTUP_FILE = $(TARGET_DIR)/s/startup-sequence

EXAMPLES = $(wildcard $(EXAMPLES_DIR)/*.c)
EXES = $(addprefix $(BUILD_DIR)/,$(notdir $(EXAMPLES:.c=)))
C_SOURCES = $(wildcard src/*.c)
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
S_SOURCES = $(wildcard src/*.s)
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(S_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(S_SOURCES)))

all: $(OBJECTS) $(EXES)

clean:
	rm -rf $(BUILD_DIR) $(TARGET_DIR)

example%: $(BUILD_DIR)/example% $(TARGET_DIR)/tileset.ts $(TARGET_DIR)/level.lvl | $(TARGET_DIR)
	cp $< $(TARGET_DIR)
	echo sys:$@ > $(TARGET_DIR)/s/startup-sequence
	fs-uae --hard_drive_0=uae/dh0 --automatic_input_grab=0

$(BUILD_DIR)/example%: $(EXAMPLES_DIR)/example%.c $(OBJECTS) | $(BUILD_DIR)
	vc $(CONFIG) -lamiga -lauto -g -I$(NDK_INC) -Isrc -o $@ $^

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	vc $(CONFIG) -c99 -g -c -I$(NDK_INC) -o $@ $<

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	vc $(CONFIG) -g -c -o $@ $<

$(TARGET_DIR)/tileset.ts $(TARGET_DIR)/level.lvl &: $(ASSETS_DIR)/tileset.json $(ASSETS_DIR)/map.json | $(TARGET_DIR)
	ratr0-converttiled $^ $(TARGET_DIR)/tileset.ts $(TARGET_DIR)/level.lvl

$(BUILD_DIR):
	mkdir -p $@

$(TARGET_DIR):
	mkdir -p $@/s
