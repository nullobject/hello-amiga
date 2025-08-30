#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <graphics/gfxbase.h>
#include <hardware/custom.h>

#include "common.h"

#define PRA_FIR0_BIT (1 << 6)

volatile uint8_t *ciaa_pra = (volatile uint8_t *)0xbfe001;
volatile uint32_t *custom_vposr = (volatile uint32_t *)0xdff004;

extern struct Custom custom;
extern struct GfxBase *GfxBase;

bool init_display(void) {
  LoadView(NULL); // clear display, reset hardware registers
  WaitTOF();      // 2 WaitTOFs to wait for 1. long frame and
  WaitTOF();      // 2. short frame copper lists to finish (if interlaced)
  return (((struct GfxBase *)GfxBase)->DisplayFlags & PAL) == PAL;
}

void reset_display(void) {
  LoadView(((struct GfxBase *)GfxBase)->ActiView);
  WaitTOF();
  WaitTOF();
  custom.cop1lc = (uint32_t)((struct GfxBase *)GfxBase)->copinit;
  RethinkDisplay();
}

void wait_vblank(uint16_t pos) {
  while (((*custom_vposr) & 0x1ff00) != (pos << 8))
    ;
}

void wait_mouse() {
  while ((*ciaa_pra & PRA_FIR0_BIT) != 0)
    ;
}
