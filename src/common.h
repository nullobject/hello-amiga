#ifndef COMMON_H
#define COMMON_H

#include <exec/types.h>

// copper instruction macros
#define COP_MOVE(addr, data) addr, data
#define COP_WAIT_END 0xffff, 0xfffe

/**
 * Initialises the display.
 */
BOOL init_display();

/**
 * Resets the display.
 */
void reset_display();

/**
 * Waits for a vertical blank.
 */
void wait_vblank(int pos);

/**
 * Waits for a mouse button
 */
void wait_mouse();

#endif
