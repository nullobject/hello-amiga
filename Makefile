BUILD_DIR=build

CC=vc
AS=vasmm68k_mot
VLINK=vlink
LDFLAGS=-stdlib
CONFIG=+kick13

EXE=uae/dh0/hello
_OBJ=hello.o mul_by_ten.o
OBJ=$(patsubst %,$(BUILD_DIR)/%,$(_OBJ))

all: $(EXE)

run: $(EXE)
	fs-uae --hard_drive_0=uae/dh0 --automatic_input_grab=0

clean:
	rm -rf $(BUILD_DIR) $(EXE)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CONFIG) -I$(NDK_INC) -g -c -k -o $@ $<

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(AS) -quiet -m68000 -Fhunk -linedebug -o $@ $<

$(EXE): $(OBJ)
	$(CC) $(CONFIG) -g -o $@ $^

$(BUILD_DIR):
	mkdir $@
