BUILD_DIR = build
TARGET = uae/dh0/hello
CC = vc
CONFIG = +kick13

C_SOURCES = $(wildcard src/*.c)
S_SOURCES = $(wildcard src/*.s)
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(S_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(S_SOURCES)))

all: $(TARGET)

run: $(TARGET)
	fs-uae --hard_drive_0=uae/dh0 --automatic_input_grab=0

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CONFIG) -c99 -g -c -I$(NDK_INC) -o $@ $<

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(CC) $(CONFIG) -g -c -o $@ $<

$(TARGET): $(OBJECTS)
	$(CC) $(CONFIG) -g -o $@ $^

$(BUILD_DIR):
	mkdir $@

.PHONY: all clean run
