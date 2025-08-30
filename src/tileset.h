#ifndef TILESET_H
#define TILESET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FILE_ID_LEN 8
#define MAX_PALETTE_SIZE 32

// information about a tile sheet
// File format version 2
// changes to version 1:
//   1. dropped the reserved2 word after palette_size
//   2. changed size of checksum from ULONG to uint16_t
struct Ratr0TilesetHeader {
  uint8_t id[FILE_ID_LEN];
  uint8_t version, flags;
  uint8_t reserved1, bmdepth;
  uint16_t width, height;
  uint16_t tile_width, tile_height;
  uint16_t num_tiles_h, num_tiles_v;
  uint16_t palette_size;
  size_t imgdata_size;
  uint16_t checksum;
};

struct Ratr0Tileset {
  struct Ratr0TilesetHeader header;
  uint16_t palette[MAX_PALETTE_SIZE];
  uint8_t *imgdata;
};

/**
 * Reads the image information from specified RATR0 tile sheet file.
 *
 * @param filename Path to the tileset file.
 * @param tileset Pointer to a Ratr0Tileset structure.
 */
bool ratr0_read_tileset(const char *filename, struct Ratr0Tileset *tileset);

/**
 * Frees the memory that was allocated for the specified RATR0 tile sheet.
 *
 * @param tileset pointer to a Ratr0Tileset structure
 */
void ratr0_free_tileset_data(struct Ratr0Tileset *tileset);

/**
 * Blits a tile to the destination buffer.
 *
 * @param tx The tileset column.
 * @param ty The tileset row.
 */
void ratr0_blit_tile(uint8_t *dst, uint16_t dmod, struct Ratr0Tileset *tileset, uint16_t tx, uint16_t ty);

struct Ratr0LevelHeader {
  uint8_t id[FILE_ID_LEN];
  uint8_t version, flags;
  uint16_t width, height;
  uint16_t checksum;
};

struct Ratr0Level {
  struct Ratr0LevelHeader header;
  uint8_t *lvldata;
};

/**
 * Reads the data from the specified RATR0 level file.
 *
 * @param filename Path to the level file.
 * @param level Pointer to a Ratr0Level structure.
 */
bool ratr0_read_level(const char *filename, struct Ratr0Level *level);

/**
 * Frees the memory that was allocated for the specified RATR0 tile sheet.
 */
void ratr0_free_level_data(struct Ratr0Level *level);

#endif
