/**
 * 3BPP tilemap with BoBs (blitter objects).
 */

#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <stdio.h>

#include "ahpc_registers.h"
#include "common.h"
#include "tileset.h"

// 20 instead of 127 because of input.device priority
#define TASK_PRIORITY 20

#define DIWSTRT_VALUE 0x2c81
#define DIWSTOP_VALUE_PAL 0x2cc1
#define DIWSTOP_VALUE_NTSC 0xf4c1

#define DDFSTRT_VALUE 0x0038
#define DDFSTOP_VALUE 0x00d0

#define BPLCON0_VALUE 0x3200
#define BPLCON1_VALUE 0x0000
#define BPLCON2_VALUE 0x0048

#define COPLIST_IDX_DIWSTOP_VALUE (9)
#define COPLIST_IDX_BPLCON1_VALUE (COPLIST_IDX_DIWSTOP_VALUE + 4)
#define COPLIST_IDX_BPLCON2_VALUE (COPLIST_IDX_BPLCON1_VALUE + 2)
#define COPLIST_IDX_BPL1MOD_VALUE (COPLIST_IDX_BPLCON2_VALUE + 2)
#define COPLIST_IDX_BPL2MOD_VALUE (COPLIST_IDX_BPL1MOD_VALUE + 2)
#define COPLIST_IDX_COLOR00_VALUE (COPLIST_IDX_BPL2MOD_VALUE + 2)
#define COPLIST_IDX_COLOR08_VALUE (COPLIST_IDX_COLOR00_VALUE + 16)
#define COPLIST_IDX_BPL1PTH_VALUE (COPLIST_IDX_COLOR00_VALUE + 64)
#define COPLIST_IDX_BPL2PTH_VALUE (COPLIST_IDX_BPL1PTH_VALUE + 4)

#define NUM_BITPLANES 3
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 256
#define HTILES (320 / 16)
#define VTILES (256 / 16)
#define BYTES_PER_ROW (SCREEN_WIDTH / 8)
#define BPL_MODULO (BYTES_PER_ROW * (NUM_BITPLANES - 1))
#define PLANE_SIZE (BYTES_PER_ROW * SCREEN_HEIGHT)
#define BUFFER_SIZE (PLANE_SIZE * NUM_BITPLANES)
#define DMOD (BYTES_PER_ROW - 2)

extern struct Custom custom;

struct Ratr0Tileset bobs;
struct Ratr0Tileset tileset;
struct Ratr0Level level;

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
    COP_MOVE(BPL6PTH, 0), COP_MOVE(BPL6PTL, 0),

    COP_MOVE(COLOR00, 0x468), 0x3c01, 0xff00, COP_MOVE(COLOR00, 0x479), 0x4c01, 0xff00, COP_MOVE(COLOR00, 0x48a),
    0x5c01, 0xff00, COP_MOVE(COLOR00, 0x49b),

    COP_WAIT_END};

void cleanup() {
  ratr0_free_tileset_data(&bobs);
  ratr0_free_tileset_data(&tileset);
  reset_display();
}

void blit_column(uint8_t *dst, short lx) {
  uint8_t *p = dst;

  for (short ly = 0; ly < VTILES; ly++) {
    short tile = level.lvldata[ly * level.header.width + lx] - 1;
    short tx = tile % tileset.header.num_tiles_h;
    short ty = tile / tileset.header.num_tiles_h;
    ratr0_blit_tile(p, DMOD, &tileset, tx, ty);
    p += BYTES_PER_ROW * tileset.header.tile_height * tileset.header.bmdepth;
  }
}

void blit_object(struct Ratr0Tileset *bobs, uint8_t *dst, short tilex, short tiley, short dstx, short dsty) {
  // actual object width (without the padding)
  short tile_width_pixels = bobs->header.tile_width - 16;

  // this tile's x-position relative to the word containing it
  short tile_x0 = bobs->header.tile_width * tilex & 0x0f;

  // 1. determine how wide the blit actually is
  short blit_width = tile_width_pixels / 16;

  // width not a multiple of 16 ? -> add 1 to the width
  if (tile_width_pixels & 0x0f)
    blit_width++;

  short blit_width0_pixels = blit_width * 16; // blit width in pixels

  // Final source blit width: does the tile extend into an additional word ?
  short src_blit_width = blit_width;
  if (tile_x0 > blit_width0_pixels - tile_width_pixels)
    src_blit_width++;

  // 2. Determine the amount of shift and the first word in the
  // destination
  short dst_x0 = dstx & 0x0f;         // destination x relative to the containing word
  short dst_shift = dst_x0 - tile_x0; // shift amount
  short dst_blit_width = blit_width;
  short dst_offset = 0;

  // negative shift => shift is to the left, so we extend the shift to the
  // left and right-shift in the previous word so we always right-shift
  if (dst_shift < 0) {
    dst_shift = 16 + dst_shift;
    dst_blit_width++;
    dst_offset = -2;
  }

  // make the blit wider if it needs more space
  if (dst_x0 > blit_width0_pixels - tile_width_pixels) {
    dst_blit_width++;
  }

  uint16_t alwm = 0xffff;
  short final_blit_width = src_blit_width;

  // due to relative positioning and shifts, the destination blit width
  // can be larger than the source blit, so we use the larger of the 2
  // and mask out last word of the source
  if (dst_blit_width > src_blit_width) {
    final_blit_width = dst_blit_width;
    alwm = 0;
  }

  WaitBlit();

  custom.bltafwm = 0xffff;
  custom.bltalwm = alwm;

  // cookie cut enable channels B, C and D, LF => D = AB + ~AC => 0xca
  // A = Mask sheet
  // B = Tile sheet
  // C = Background
  // D = Background
  custom.bltcon0 = 0x0fca | (dst_shift << 12);
  custom.bltcon1 = dst_shift << 12; // shift in B

  // modulos are in bytes
  uint16_t srcmod = bobs->header.width / 8 - (final_blit_width * 2);
  uint16_t dstmod = 320 / 8 - (final_blit_width * 2);
  custom.bltamod = srcmod;
  custom.bltbmod = srcmod;
  custom.bltcmod = dstmod;
  custom.bltdmod = dstmod;

  // The blit size is the size of a plane of the tile size (1 word * 16)
  uint16_t bltsize = (bobs->header.tile_height << 6) | (final_blit_width & 0x3f);

  // map the tile position to physical coordinates in the tile sheet
  short srcx = tilex * bobs->header.tile_width;
  short srcy = tiley * bobs->header.tile_height;

  short bobs_plane_size = bobs->header.width / 8 * bobs->header.height;

  uint8_t *src = bobs->imgdata + srcy * bobs->header.width / 8 + srcx / 8;
  // The mask data is the plane after the source image planes
  uint8_t *mask = bobs->imgdata + bobs_plane_size * bobs->header.bmdepth + srcy * bobs->header.width / 8 + srcx / 8;
  uint8_t *p = dst + dsty * 320 / 8 + dstx / 8 + dst_offset;

  for (short i = 0; i < bobs->header.bmdepth; i++) {
    custom.bltapt = mask;
    custom.bltbpt = src;
    custom.bltcpt = p;
    custom.bltdpt = p;
    custom.bltsize = bltsize;

    // Increase the pointers to the next plane
    src += bobs_plane_size;
    p += BYTES_PER_ROW * bobs->header.tile_height;

    WaitBlit();
  }
}

int main(int argc, char **argv) {
  SetTaskPri(FindTask(NULL), TASK_PRIORITY);
  bool is_pal = init_display();

  uint8_t __chip *fg_buffer = AllocMem(BUFFER_SIZE, MEMF_CHIP | MEMF_CLEAR);
  uint8_t __chip *bg_buffer = AllocMem(BUFFER_SIZE, MEMF_CHIP | MEMF_CLEAR);

  if (!ratr0_read_tileset("rodland_bobs.ts", &bobs)) {
    puts("Could not read bobs");
    cleanup();
    return 1;
  }

  if (!ratr0_read_tileset("8c-tileset.ts", &tileset)) {
    puts("Could not read tileset");
    cleanup();
    return 1;
  }

  if (!ratr0_read_level("8c-level.lvl", &level)) {
    puts("Could not read level");
    cleanup();
    return 1;
  }

  if (is_pal) {
    coplist[COPLIST_IDX_DIWSTOP_VALUE] = DIWSTOP_VALUE_PAL;
  } else {
    coplist[COPLIST_IDX_DIWSTOP_VALUE] = DIWSTOP_VALUE_NTSC;
  }

  short num_colors = 1 << tileset.header.bmdepth;
  for (short i = 0; i < num_colors; i++) {
    coplist[COPLIST_IDX_COLOR00_VALUE + (i << 1)] = bobs.palette[i];
    coplist[COPLIST_IDX_COLOR08_VALUE + (i << 1)] = bobs.palette[i];
  }

  short coplist_idx = COPLIST_IDX_BPL1PTH_VALUE;
  uint32_t addr = (uint32_t)fg_buffer;
  for (short i = 0; i < NUM_BITPLANES; i++) {
    coplist[coplist_idx] = (addr >> 16) & 0xffff;
    coplist[coplist_idx + 2] = addr & 0xffff;
    coplist_idx += 8; // next bitplane
    addr += BYTES_PER_ROW;
    // addr += PLANE_SIZE;
  }

  coplist_idx = COPLIST_IDX_BPL2PTH_VALUE;
  addr = (uint32_t)bg_buffer;
  for (short i = 0; i < NUM_BITPLANES; i++) {
    coplist[coplist_idx] = (addr >> 16) & 0xffff;
    coplist[coplist_idx + 2] = addr & 0xffff;
    coplist_idx += 8; // next bitplane
    addr += BYTES_PER_ROW;
    // addr += PLANE_SIZE;
  }

  // Disable sprite DMA
  custom.dmacon = DMAF_SPRITE;

  OwnBlitter();

  for (short lx = 0; lx < HTILES; lx++) {
    blit_column(bg_buffer + lx * 2, lx);
  }

  blit_object(&bobs, fg_buffer, 0, 0, 64, 64);

  DisownBlitter();

  // Apply copper list
  custom.cop1lc = (uint32_t)coplist;

  wait_mouse();

  cleanup();

  return 0;
}
