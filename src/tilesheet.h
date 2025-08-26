#ifndef TILESHEET_H
#define TILESHEET_H

#include <exec/types.h>

#define FILE_ID_LEN 8
#define MAX_PALETTE_SIZE 32

// information about a tile sheet
// File format version 2
// changes to version 1:
//   1. dropped the reserved2 word after palette_size
//   2. changed size of checksum from ULONG to UWORD
struct Ratr0TileSheetHeader {
  UBYTE id[FILE_ID_LEN];
  UBYTE version, flags;
  UBYTE reserved1, bmdepth;
  UWORD width, height;
  UWORD tile_width, tile_height;
  UWORD num_tiles_h, num_tiles_v;
  UWORD palette_size;
  ULONG imgdata_size;
  UWORD checksum;
};

struct Ratr0TileSheet {
  struct Ratr0TileSheetHeader header;
  UWORD palette[MAX_PALETTE_SIZE];
  UBYTE *imgdata;
};

extern ULONG ratr0_read_tilesheet(const char *filename, struct Ratr0TileSheet *sheet);
extern void ratr0_free_tilesheet_data(struct Ratr0TileSheet *sheet);
extern void ratr0_blit_tile(UBYTE *dst, int dmod, struct Ratr0TileSheet *tileset, int tx, int ty);

struct Ratr0LevelHeader {
  UBYTE id[FILE_ID_LEN];
  UBYTE version, flags;
  UWORD width, height;
  UWORD checksum;
};

struct Ratr0Level {
  struct Ratr0LevelHeader header;
  UBYTE *lvldata;
};

extern BOOL ratr0_read_level(const char *filename, struct Ratr0Level *level);
extern void ratr0_free_level_data(struct Ratr0Level *level);

#endif
