#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdint.h>

// copper instruction macros
#define COP_MOVE(addr, data) addr, data
#define COP_WAIT_END 0xffff, 0xfffe

/**
 * Initialises the display.
 */
bool init_display();

/**
 * Resets the display.
 */
void reset_display();

/**
 * Waits for a raster line.
 */
void wait_line(uint16_t pos);

/**
 * Waits for a mouse button
 */
void wait_mouse();

#endif
