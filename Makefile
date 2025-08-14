CC=vc
AS=vasmm68k_mot
VLINK=vlink

BUILD_DIR=build
CONFIG=+kick13
CC_FLAGS=-c99 -g -c -I$(NDK_INC)
AS_FLAGS=-quiet -m68000 -Fhunk -linedebug
EXE=uae/dh0/hello

_OBJ=hello.o mul_by_ten.o
OBJ=$(patsubst %,$(BUILD_DIR)/%,$(_OBJ))

all: $(EXE)

run: $(EXE)
	fs-uae --hard_drive_0=uae/dh0 --automatic_input_grab=0

clean:
	rm -rf $(BUILD_DIR) $(EXE)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CONFIG) $(CC_FLAGS) -o $@ $<

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(AS) $(AS_FLAGS) -o $@ $<

$(EXE): $(OBJ)
	$(CC) $(CONFIG) -g -o $@ $^

$(BUILD_DIR):
	mkdir $@

.PHONY: all clean run
