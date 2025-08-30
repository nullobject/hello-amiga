#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <graphics/gfxbase.h>
#include <hardware/custom.h>

#include "common.h"

#define PRA_FIR0_BIT (1 << 6)

extern struct Custom custom;
extern struct GfxBase *GfxBase;

BOOL init_display(void) {
  LoadView(NULL); // clear display, reset hardware registers
  WaitTOF();      // 2 WaitTOFs to wait for 1. long frame and
  WaitTOF();      // 2. short frame copper lists to finish (if interlaced)
  return (((struct GfxBase *)GfxBase)->DisplayFlags & PAL) == PAL;
}

void reset_display(void) {
  LoadView(((struct GfxBase *)GfxBase)->ActiView);
  WaitTOF();
  WaitTOF();
  custom.cop1lc = (ULONG)((struct GfxBase *)GfxBase)->copinit;
  RethinkDisplay();
}

void wait_mouse() {
  volatile UBYTE *ciaa_pra = (volatile UBYTE *)0xbfe001;
  while ((*ciaa_pra & PRA_FIR0_BIT) != 0)
    ;
}
