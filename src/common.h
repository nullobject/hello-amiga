#ifndef COMMON_H
#define COMMON_H

#include <exec/types.h>

/**
 * Initialises the display.
 */
BOOL init_display();

/**
 * Resets the display.
 */
void reset_display();

/**
 * Waits for a mouse button
 */
void wait_mouse();

#endif
