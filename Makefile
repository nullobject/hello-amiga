.PHONY: all clean
.SECONDARY:

ASSETS_DIR = assets
BUILD_DIR = build
TARGET_DIR = uae/dh0

CONFIG = +kick13
TARGET = $(TARGET_DIR)/app.exe

EXAMPLES = $(wildcard examples/*.c)
vpath %.c $(sort $(dir $(EXAMPLES)))
C_SOURCES = $(wildcard src/*.c)
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
S_SOURCES = $(wildcard src/*.s)
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(S_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(S_SOURCES)))

all: $(OBJECTS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TARGET_DIR)/tileset.ts $(TARGET_DIR)/level.lvl

example%: $(BUILD_DIR)/example%.exe $(TARGET_DIR)/tileset.ts $(TARGET_DIR)/level.lvl
	cp $< $(TARGET)
	fs-uae --hard_drive_0=uae/dh0 --automatic_input_grab=0

$(BUILD_DIR)/%.exe: %.c $(OBJECTS) | $(BUILD_DIR)
	vc $(CONFIG) -lamiga -lauto -g -I$(NDK_INC) -o $@ $^

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	vc $(CONFIG) -c99 -g -c -I$(NDK_INC) -o $@ $<

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	vc $(CONFIG) -g -c -o $@ $<

$(TARGET_DIR)/tileset.ts $(TARGET_DIR)/level.lvl &: $(ASSETS_DIR)/tileset.json $(ASSETS_DIR)/map.json
	ratr0-converttiled $^ $(TARGET_DIR)/tileset.ts $(TARGET_DIR)/level.lvl

$(BUILD_DIR):
	mkdir $@
