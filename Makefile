.PHONY: clean
.SECONDARY:

BUILD_DIR = build
TARGET_DIR = uae/dh0
TARGET = uae/dh0/app.exe
CC = vc
CONFIG = +kick13

EXAMPLES = $(wildcard examples/*.c)
vpath %.c $(sort $(dir $(EXAMPLES)))
C_SOURCES = $(wildcard src/*.c)
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
S_SOURCES = $(wildcard src/*.s)
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(S_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(S_SOURCES)))

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

example%: $(BUILD_DIR)/example%.exe
	cp $< $(TARGET)
	fs-uae --hard_drive_0=uae/dh0 --automatic_input_grab=0

$(BUILD_DIR)/%.exe: %.c $(OBJECTS) | $(BUILD_DIR)
	$(CC) $(CONFIG) -lamiga -lauto -g -I$(NDK_INC) -o $@ $^

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CONFIG) -c99 -g -c -I$(NDK_INC) -o $@ $<

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(CC) $(CONFIG) -g -c -o $@ $<

$(BUILD_DIR):
	mkdir $@
