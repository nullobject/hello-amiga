CC = vc
BUILD_DIR = build
CONFIG = +kick13
EXE = uae/dh0/hello
_OBJ = hello.o mul_by_ten.o
OBJ = $(patsubst %,$(BUILD_DIR)/%,$(_OBJ))

all: $(EXE)

run: $(EXE)
	fs-uae --hard_drive_0=uae/dh0 --automatic_input_grab=0

clean:
	rm -rf $(BUILD_DIR) $(EXE)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CONFIG) -c99 -g -c -I$(NDK_INC) -o $@ $<

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	$(CC) $(CONFIG) -g -c -o $@ $<

$(EXE): $(OBJ)
	$(CC) $(CONFIG) -g -o $@ $^

$(BUILD_DIR):
	mkdir $@

.PHONY: all clean run
