#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>
#include <hardware/custom.h>
#include <stdio.h>
#include <stdlib.h>

#include "tileset.h"

extern struct Custom custom;

BOOL ratr0_read_tileset(const char *filename, struct Ratr0Tileset *tileset) {
  FILE *fp = fopen(filename, "rb");

  if (fp) {
    size_t elems_read = fread(&tileset->header, sizeof(struct Ratr0TilesetHeader), 1, fp);
    elems_read = fread(&tileset->palette, sizeof(UWORD), tileset->header.palette_size, fp);
    tileset->imgdata = AllocMem(tileset->header.imgdata_size, MEMF_CHIP | MEMF_CLEAR);
    elems_read = fread(tileset->imgdata, sizeof(unsigned char), tileset->header.imgdata_size, fp);
    fclose(fp);
    return TRUE;
  } else {
    printf("ratr0_read_tileset() error: file '%s' not found\n", filename);
    return FALSE;
  }
}

void ratr0_free_tileset_data(struct Ratr0Tileset *tileset) {
  if (tileset && tileset->imgdata)
    FreeMem(tileset->imgdata, tileset->header.imgdata_size);
}

void ratr0_blit_tile(UBYTE *dst, UWORD dmod, struct Ratr0Tileset *tileset, UWORD tx, UWORD ty) {
  UWORD height = tileset->header.tile_height * tileset->header.bmdepth;
  UWORD num_words = 1;

  // map tilenum to offset
  UWORD tile_row_bytes = tileset->header.num_tiles_h * 2 * tileset->header.tile_width * tileset->header.bmdepth;
  UWORD src_offset = ty * tile_row_bytes + tx * 2;

  WaitBlit();
  custom.bltcon0 = 0x9f0; // enable channels A and D, LF => D = A
  custom.bltcon1 = 0;     // copy direction: asc
  custom.bltapt = tileset->imgdata + src_offset;
  custom.bltdpt = dst;
  custom.bltamod = (tileset->header.num_tiles_h - 1) * 2;
  custom.bltdmod = dmod;
  // copy everything, bltafwm and bltalwm are all set to 1's
  custom.bltafwm = 0xffff;
  custom.bltalwm = 0xffff;

  // B and C are disabled, just set their data registers to all 1's
  custom.bltbdat = 0xffff;
  custom.bltcdat = 0xffff;

  custom.bltsize = (UWORD)(height << 6) | (num_words & 0x3f);
}

BOOL ratr0_read_level(const char *filename, struct Ratr0Level *level) {
  FILE *fp = fopen(filename, "rb");

  if (fp) {
    size_t elems_read = fread(&level->header, sizeof(struct Ratr0LevelHeader), 1, fp);
    size_t data_size = sizeof(UBYTE) * level->header.width * level->header.height;
    level->lvldata = malloc(data_size);
    elems_read = fread(level->lvldata, sizeof(unsigned char), data_size, fp);
    fclose(fp);
    return TRUE;
  } else {
    printf("ratr0_read_level() error: file '%s' not found\n", filename);
    return FALSE;
  }
}

void ratr0_free_level_data(struct Ratr0Level *level) {
  if (level && level->lvldata) {
    free(level->lvldata);
  }
}
