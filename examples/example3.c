#include <clib/exec_protos.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>

#include "../src/ahpc_registers.h"
#include "../src/common.h"
#include "../src/tileset.h"

// 20 instead of 127 because of input.device priority
#define TASK_PRIORITY 20

#define DIWSTRT_VALUE 0x2c81
#define DIWSTOP_VALUE_PAL 0x2cc1
#define DIWSTOP_VALUE_NTSC 0xf4c1

#define DDFSTRT_VALUE 0x0038
#define DDFSTOP_VALUE 0x00d0

#define BPLCON0_VALUE 0x5200
#define BPLCON1_VALUE 0x0000
#define BPLCON2_VALUE 0x0048

#define COPLIST_IDX_DIWSTOP_VALUE (9)
#define COPLIST_IDX_BPLCON1_VALUE (COPLIST_IDX_DIWSTOP_VALUE + 4)
#define COPLIST_IDX_BPLCON2_VALUE (COPLIST_IDX_BPLCON1_VALUE + 2)
#define COPLIST_IDX_BPL1MOD_VALUE (COPLIST_IDX_BPLCON2_VALUE + 2)
#define COPLIST_IDX_BPL2MOD_VALUE (COPLIST_IDX_BPL1MOD_VALUE + 2)
#define COPLIST_IDX_COLOR00_VALUE (COPLIST_IDX_BPL2MOD_VALUE + 2)
#define COPLIST_IDX_BPL1PTH_VALUE (COPLIST_IDX_COLOR00_VALUE + 64)

#define NUM_BITPLANES 5
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 256
#define HTILES (320 / 16)
#define VTILES (256 / 16)
#define BYTES_PER_ROW (SCREEN_WIDTH / 8)
#define BPL_MODULO (BYTES_PER_ROW * (NUM_BITPLANES - 1))
#define PLANE_SIZE (BYTES_PER_ROW * SCREEN_HEIGHT)
#define DMOD (BYTES_PER_ROW - 2)

extern struct Custom custom;

struct Ratr0Tileset tileset;

uint16_t __chip coplist[] = {
    // set fetch mode = 0
    COP_MOVE(FMODE, 0),

    // set display registers
    COP_MOVE(DDFSTRT, DDFSTRT_VALUE), COP_MOVE(DDFSTOP, DDFSTOP_VALUE), COP_MOVE(DIWSTRT, DIWSTRT_VALUE),
    COP_MOVE(DIWSTOP, DIWSTOP_VALUE_PAL), COP_MOVE(BPLCON0, BPLCON0_VALUE), COP_MOVE(BPLCON1, BPLCON1_VALUE),
    COP_MOVE(BPLCON2, BPLCON2_VALUE), COP_MOVE(BPL1MOD, BPL_MODULO), COP_MOVE(BPL2MOD, BPL_MODULO),

    // set color registers
    COP_MOVE(COLOR00, 0x000), COP_MOVE(COLOR01, 0x000), COP_MOVE(COLOR02, 0x000), COP_MOVE(COLOR03, 0x000),
    COP_MOVE(COLOR04, 0x000), COP_MOVE(COLOR05, 0x000), COP_MOVE(COLOR06, 0x000), COP_MOVE(COLOR07, 0x000),
    COP_MOVE(COLOR08, 0x000), COP_MOVE(COLOR09, 0x000), COP_MOVE(COLOR10, 0x000), COP_MOVE(COLOR11, 0x000),
    COP_MOVE(COLOR12, 0x000), COP_MOVE(COLOR13, 0x000), COP_MOVE(COLOR14, 0x000), COP_MOVE(COLOR15, 0x000),
    COP_MOVE(COLOR16, 0x000), COP_MOVE(COLOR17, 0x000), COP_MOVE(COLOR18, 0x000), COP_MOVE(COLOR19, 0x000),
    COP_MOVE(COLOR20, 0x000), COP_MOVE(COLOR21, 0x000), COP_MOVE(COLOR22, 0x000), COP_MOVE(COLOR23, 0x000),
    COP_MOVE(COLOR24, 0x000), COP_MOVE(COLOR25, 0x000), COP_MOVE(COLOR26, 0x000), COP_MOVE(COLOR27, 0x000),
    COP_MOVE(COLOR28, 0x000), COP_MOVE(COLOR29, 0x000), COP_MOVE(COLOR30, 0x000), COP_MOVE(COLOR31, 0x000),

    // set bitplane registers
    COP_MOVE(BPL1PTH, 0), COP_MOVE(BPL1PTL, 0), COP_MOVE(BPL2PTH, 0), COP_MOVE(BPL2PTL, 0), COP_MOVE(BPL3PTH, 0),
    COP_MOVE(BPL3PTL, 0), COP_MOVE(BPL4PTH, 0), COP_MOVE(BPL4PTL, 0), COP_MOVE(BPL5PTH, 0), COP_MOVE(BPL5PTL, 0),

    COP_WAIT_END};

void cleanup(void) {
  ratr0_free_tileset_data(&tileset);
  reset_display();
}

void blit_column(uint8_t *dst, uint16_t tile) {
  uint8_t *p = dst;

  for (int ly = 0; ly < VTILES; ly++) {
    uint16_t tx = tile % tileset.header.num_tiles_h;
    uint16_t ty = tile / tileset.header.num_tiles_h;
    ratr0_blit_tile(p, DMOD, &tileset, tx, ty);
    p += BYTES_PER_ROW * tileset.header.tile_height * tileset.header.bmdepth;
  }
}

int main(int argc, char **argv) {
  SetTaskPri(FindTask(NULL), TASK_PRIORITY);
  bool is_pal = init_display();

  size_t display_buffer_size = PLANE_SIZE * NUM_BITPLANES;
  uint8_t __chip *display_buffer = AllocMem(display_buffer_size, MEMF_CHIP | MEMF_CLEAR);

  if (!ratr0_read_tileset("graphics/rocknroll_tiles.ts", &tileset)) {
    puts("Could not read tile set");
    cleanup();
    return 1;
  }

  if (is_pal) {
    coplist[COPLIST_IDX_DIWSTOP_VALUE] = DIWSTOP_VALUE_PAL;
  } else {
    coplist[COPLIST_IDX_DIWSTOP_VALUE] = DIWSTOP_VALUE_NTSC;
  }

  uint8_t num_colors = 1 << tileset.header.bmdepth;
  for (int i = 0; i < num_colors; i++) {
    coplist[COPLIST_IDX_COLOR00_VALUE + (i << 1)] = tileset.palette[i];
  }

  int coplist_idx = COPLIST_IDX_BPL1PTH_VALUE;
  uint32_t addr = (uint32_t)display_buffer;
  for (int i = 0; i < NUM_BITPLANES; i++) {
    coplist[coplist_idx] = (addr >> 16) & 0xffff;
    coplist[coplist_idx + 2] = addr & 0xffff;
    coplist_idx += 4; // next bitplane
    addr += BYTES_PER_ROW;
  }
  OwnBlitter();

  for (uint16_t lx = 0; lx < HTILES; lx++) {
    blit_column(display_buffer + lx * 2, lx);
  }

  // Disable sprite DMA
  custom.dmacon = DMAF_SPRITE;

  // Apply copper list
  custom.cop1lc = (uint32_t)coplist;

  // Wait for mouse button
  wait_mouse();

  DisownBlitter();
  FreeMem(display_buffer, display_buffer_size);
  cleanup();

  return 0;
}
