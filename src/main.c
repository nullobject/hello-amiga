#include <exec/exec.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>

#ifdef __MAXON__
#include <pragma/exec_lib.h>
#include <pragma/dos_lib.h>
#include <pragma/graphics_lib.h>
#include <pragma/intuition_lib.h>
#else
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#endif

#include <stdio.h>
#include <string.h>

#include "hardware.h"
#include "cop.h"
#include "map.h"


#define ARG_TEMPLATE "SPEED/S,NTSC/S,HOW/S,SKY/S,FMODE/N/K"
#define ARG_SPEED 0
#define ARG_NTSC  1
#define ARG_HOW   2
#define ARG_SKY   3
#define ARG_FMODE 4
#define NUM_ARGS  5

#define MAPNAME		"large.raw"
#define BLOCKSNAME	"demoblocks.raw"

#define SCREENWIDTH  320
#define SCREENHEIGHT 256
#define EXTRAWIDTH 32
#define SCREENBYTESPERROW (SCREENWIDTH / 8)

#define BITMAPWIDTH (SCREENWIDTH + EXTRAWIDTH)
#define BITMAPBYTESPERROW (BITMAPWIDTH / 8)
#define BITMAPHEIGHT SCREENHEIGHT

#define BLOCKSWIDTH 320
#define BLOCKSHEIGHT 256
#define BLOCKSDEPTH 4
#define BLOCKSCOLORS (1L << BLOCKSDEPTH)
#define BLOCKWIDTH 16
#define BLOCKHEIGHT 16
#define BLOCKSBYTESPERROW (BLOCKSWIDTH / 8)
#define BLOCKSPERROW (BLOCKSWIDTH / BLOCKWIDTH)

#define NUMSTEPS BLOCKWIDTH

#define BITMAPBLOCKSPERROW (BITMAPWIDTH / BLOCKWIDTH)
#define BITMAPBLOCKSPERCOL (BITMAPHEIGHT / BLOCKHEIGHT)

#define BITMAPPLANELINES (BITMAPHEIGHT * BLOCKSDEPTH)
#define BLOCKPLANELINES  (BLOCKHEIGHT * BLOCKSDEPTH)

#define DIWSTART 0x2981
#define DIWSTOP  0x29C1

#define PALSIZE (BLOCKSCOLORS * 2)
#define BLOCKSFILESIZE (BLOCKSWIDTH * BLOCKSHEIGHT * frontbuffer / 8 + PALSIZE)

#define DIRECTION_RIGHT 0
#define DIRECTION_LEFT  1


struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;
struct Screen *scr;
struct RastPort *ScreenRastPort;
struct BitMap *BlocksBitmap,*ScreenBitmap;
struct RawMap *Map;
UBYTE	 *frontbuffer,*blocksbuffer;

WORD	mapposx,videoposx;
WORD	bitmapheight;
BYTE	previous_direction;
WORD	*savewordpointer;
WORD	saveword;

LONG	mapwidth,mapheight;
UBYTE *mapdata;

UWORD	colors[BLOCKSCOLORS];

LONG	Args[NUM_ARGS];

BOOL	option_ntsc,option_how,option_speed,option_sky;
WORD	option_fetchmode;

BPTR	MyHandle;
char	s[256];


#if EXTRAWIDTH == 32

	// bitmap width aligned to 32 Pixels
	#define MAX_FETCHMODE 2
	#define MAX_FETCHMODE_S "2"

#elif EXTRAWIDTH == 64

	// bitmap width aligned to 64 Pixels
	#define MAX_FETCHMODE 3
	#define MAX_FETCHMODE_S "3"

#else

	// bad extrawidth
	#error "EXTRAWIDTH must be either 32 or 64"

#endif

struct FetchInfo
{
	WORD	ddfstart;
	WORD	ddfstop;
	WORD	modulooffset;
	WORD	bitmapoffset;
	WORD	scrollpixels;
} fetchinfo [] =
{
	{0x30,0xD0,2,0,16},	/* normal         */
	{0x28,0xC8,4,16,32},	/* BPL32          */
	{0x28,0xC8,4,16,32},	/* BPAGEM         */
	{0x18,0xB8,8,48,64}	/* BPL32 + BPAGEM */
};

/********************* MACROS ***********************/

#define ROUND2BLOCKWIDTH(x) ((x) & ~(BLOCKWIDTH - 1))

/************* SETUP/CLEANUP ROUTINES ***************/

static void Cleanup (char *msg)
{
	WORD rc;
	
	if (msg)
	{
		printf("Error: %s\n",msg);
		rc = RETURN_WARN;
	} else {
		rc = RETURN_OK;
	}

	if (scr) CloseScreen(scr);

	if (ScreenBitmap)
	{
		WaitBlit();
		FreeBitMap(ScreenBitmap);
	}

	if (BlocksBitmap)
	{
		WaitBlit();
		FreeBitMap(BlocksBitmap);
	}

	if (Map) FreeVec(Map);
	if (MyHandle) Close(MyHandle);

	if (GfxBase) CloseLibrary((struct Library *)GfxBase);
	if (IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);

	exit(rc);
}

static void OpenLibs(void)
{
	if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",39)))
	{
		Cleanup("Can't open intuition.library V39!");
	}
	
	if (!(GfxBase = (struct GfxBase *)OpenLibrary("graphics.library",39)))
	{
		Cleanup("Can't open graphics.library V39!");
	}
}

static void GetArguments(void)
{
	struct RDArgs *MyArgs;

	if (!(MyArgs = ReadArgs(ARG_TEMPLATE,Args,0)))
	{
		Fault(IoErr(),0,s,255);
		Cleanup(s);
	}

	if (Args[ARG_SPEED]) option_speed = TRUE;
	if (Args[ARG_NTSC]) option_ntsc = TRUE;
	if (Args[ARG_HOW])
	{
		option_how = TRUE;
		option_speed = FALSE;
	}
	if (Args[ARG_SKY] && (!option_speed))
	{
		option_sky = TRUE;
	}

	if (Args[ARG_FMODE])
	{
		option_fetchmode = *(LONG *)Args[ARG_FMODE];
	}

	FreeArgs(MyArgs);
	
	if (option_fetchmode < 0 || option_fetchmode > MAX_FETCHMODE)
	{
		Cleanup("Invalid fetch mode. Must be 0 .. " MAX_FETCHMODE_S "!");
	}
}

static void OpenMap(void)
{
	LONG l;

	if (!(MyHandle = Open(MAPNAME,MODE_OLDFILE)))
	{
		Fault(IoErr(),0,s,255);
		Cleanup(s);
	}
	
	Seek(MyHandle,0,OFFSET_END);
	l = Seek(MyHandle,0,OFFSET_BEGINNING);

	if (!(Map = AllocVec(l,MEMF_PUBLIC)))
	{
		Cleanup("Out of memory!");
	}
	
	if (Read(MyHandle,Map,l) != l)
	{
		Fault(IoErr(),0,s,255);
		Cleanup(s);
	}
	
	Close(MyHandle);MyHandle = 0;
	
	mapdata = Map->data;
	mapwidth = Map->mapwidth;
	mapheight = Map->mapheight;
}

static void OpenBlocks(void)
{
	LONG l;

	if (!(BlocksBitmap = AllocBitMap(BLOCKSWIDTH,
											   BLOCKSHEIGHT,
											   BLOCKSDEPTH,
											   BMF_STANDARD | BMF_INTERLEAVED,
											   0)))
	{
		Cleanup("Can't alloc blocks bitmap!");
	}
	
	if (!(MyHandle = Open(BLOCKSNAME,MODE_OLDFILE)))
	{
		Fault(IoErr(),0,s,255);
		Cleanup(s);
	}
	
	if (Read(MyHandle,colors,PALSIZE) != PALSIZE)
	{
		Fault(IoErr(),0,s,255);
		Cleanup(s);
	}
	
	l = BLOCKSWIDTH * BLOCKSHEIGHT * BLOCKSDEPTH / 8;
	
	if (Read(MyHandle,BlocksBitmap->Planes[0],l) != l)
	{
		Fault(IoErr(),0,s,255);
		Cleanup(s);
	}
	
	Close(MyHandle);MyHandle = 0;
	
	blocksbuffer = BlocksBitmap->Planes[0];
}

static void OpenDisplay(void)
{	
	struct DimensionInfo diminfo;
	DisplayInfoHandle 	dih;
	ULONG						modeid;
	LONG						l;
	
	bitmapheight = BITMAPHEIGHT +
						(mapwidth / BITMAPBLOCKSPERROW / BLOCKSDEPTH) + 1 +
						3;
						

	if (!(ScreenBitmap = AllocBitMap(BITMAPWIDTH,bitmapheight,BLOCKSDEPTH,BMF_STANDARD | BMF_INTERLEAVED | BMF_CLEAR,0)))
	{
		Cleanup("Can't alloc screen bitmap!");
	}

	frontbuffer = ScreenBitmap->Planes[0];
	frontbuffer += (fetchinfo[option_fetchmode].bitmapoffset / 8);

	if (!(TypeOfMem(ScreenBitmap->Planes[0]) & MEMF_CHIP))
	{
		Cleanup("Screen bitmap is not in CHIP RAM!?? If you have a gfx card try disabling \"planes to fast\" or similiar options in your RTG system!");
	}

	l = GetBitMapAttr(ScreenBitmap,BMA_FLAGS);
	
	if (!(GetBitMapAttr(ScreenBitmap,BMA_FLAGS) & BMF_INTERLEAVED))
	{
		Cleanup("Screen bitmap is not in interleaved format!??");
	}
	
	if (option_how)
	{
		modeid = INVALID_ID;

		if ((dih = FindDisplayInfo(VGAPRODUCT_KEY)))
		{
			if (GetDisplayInfoData(dih,(APTR)&diminfo,sizeof(diminfo),DTAG_DIMS,0))
			{
				if (diminfo.MaxDepth >= BLOCKSDEPTH) modeid = VGAPRODUCT_KEY;
			}
		}
		if (modeid == INVALID_ID)
		{
			if (option_ntsc)
			{
				modeid = NTSC_MONITOR_ID | HIRESLACE_KEY;
			} else {
				modeid = PAL_MONITOR_ID | HIRESLACE_KEY;
			}
		}
	} else {
		if (option_ntsc)
		{
			modeid = NTSC_MONITOR_ID;
		} else {
			modeid = PAL_MONITOR_ID;
		}
	}

	if (!(scr = OpenScreenTags(0,SA_Width,BITMAPWIDTH,
										  SA_Height,bitmapheight,
										  SA_Depth,BLOCKSDEPTH,
										  SA_DisplayID,modeid,
										  SA_BitMap,ScreenBitmap,
										  option_how ? SA_Overscan : TAG_IGNORE,OSCAN_TEXT,
										  option_how ? SA_AutoScroll : TAG_IGNORE,TRUE,
										  SA_Quiet,TRUE,
										  TAG_DONE)))
	{
		Cleanup("Can't open screen!");
	}

	if (scr->RastPort.BitMap->Planes[0] != ScreenBitmap->Planes[0])
	{
		Cleanup("Screen was not created with the custom bitmap I supplied!??");
	}
	
	ScreenRastPort = &scr->RastPort;
	
	LoadRGB4(&scr->ViewPort,colors,BLOCKSCOLORS);
}

static void InitCopperlist(void)
{
	WORD	*wp;
	LONG l;

	WaitVBL();

	custom->dmacon = 0x7FFF;
	custom->beamcon0 = option_ntsc ? 0 : DISPLAYPAL;

	CopFETCHMODE[1] = option_fetchmode;
	
	// bitplane control registers

	CopBPLCON0[1] = ((BLOCKSDEPTH * BPL0_BPU0_F) & BPL0_BPUMASK) +
						 ((BLOCKSDEPTH / 8) * BPL0_BPU3_F) +
						 BPL0_COLOR_F +
						 (option_speed ? 0 : BPL0_USEBPLCON3_F);

	CopBPLCON1[1] = 0;

	CopBPLCON3[1] = BPLCON3_BRDNBLNK;

	// bitplane modulos

	l = BITMAPBYTESPERROW * BLOCKSDEPTH -
		 SCREENBYTESPERROW - fetchinfo[option_fetchmode].modulooffset;

	CopBPLMODA[1] = l;
	CopBPLMODB[1] = l;
	
	// display window start/stop
	
	CopDIWSTART[1] = DIWSTART;
	CopDIWSTOP[1] = DIWSTOP;
	
	// display data fetch start/stop
	
	CopDDFSTART[1] = fetchinfo[option_fetchmode].ddfstart;
	CopDDFSTOP[1]  = fetchinfo[option_fetchmode].ddfstop;
	
	// plane pointers

	wp = CopPLANE1H;

	for(l = 0;l < BLOCKSDEPTH;l++)
	{
		wp[1] = (WORD)(((ULONG)ScreenBitmap->Planes[l]) >> 16);
		wp[3] = (WORD)(((ULONG)ScreenBitmap->Planes[l]) & 0xFFFF);
		
		wp += 4;
	}

	if (option_sky)
	{
		// activate copper sky
		
		CopSKY[0] = 0x290f;
	}

	custom->intena = 0x7FFF;
	
	custom->dmacon = DMAF_SETCLR | DMAF_BLITTER | DMAF_COPPER | DMAF_RASTER | DMAF_MASTER;

	custom->cop2lc = (ULONG)CopperList;	
};

/******************* SCROLLING **********************/

static void DrawBlock(LONG x,LONG y,LONG mapx,LONG mapy)
{
	UBYTE block;

	// x = in pixels
	// y = in "planelines" (1 realline = BLOCKSDEPTH planelines)


	x = (x / 8) & 0xFFFE;
	y = y * BITMAPBYTESPERROW;
	
	block = mapdata[mapy * mapwidth + mapx];

	mapx = (block % BLOCKSPERROW) * (BLOCKWIDTH / 8);
	mapy = (block / BLOCKSPERROW) * (BLOCKPLANELINES * BLOCKSBYTESPERROW);
	
	if (option_how) OwnBlitter();

	HardWaitBlit();
	
	custom->bltcon0 = 0x9F0;	// use A and D. Op: D = A
	custom->bltcon1 = 0;
	custom->bltafwm = 0xFFFF;
	custom->bltalwm = 0xFFFF;
	custom->bltamod = BLOCKSBYTESPERROW - (BLOCKWIDTH / 8);
	custom->bltdmod = BITMAPBYTESPERROW - (BLOCKWIDTH / 8);
	custom->bltapt  = blocksbuffer + mapy + mapx;
	custom->bltdpt	 = frontbuffer + y + x;
	
	custom->bltsize = BLOCKPLANELINES * 64 + (BLOCKWIDTH / 16);

	if (option_how) DisownBlitter();
}

static void FillScreen(void)
{
	WORD a,b,x,y;
	
	for (b = 0;b < BITMAPBLOCKSPERCOL;b++)
	{
		for (a = 0;a < BITMAPBLOCKSPERROW;a++)
		{
			x = a * BLOCKWIDTH;
			y = b * BLOCKPLANELINES;
			DrawBlock(x,y,a,b);
		}
	}
}

static void ScrollLeft(void)
{
	WORD mapx,mapy,x,y;

	if (mapposx < 1) return;

	mapposx--;
	videoposx = mapposx;

	mapx = mapposx / BLOCKWIDTH;
	mapy = mapposx & (NUMSTEPS - 1);
	
	x = ROUND2BLOCKWIDTH(videoposx);
	y = mapy * BLOCKPLANELINES;
	
	if (previous_direction == DIRECTION_RIGHT)
	{
		HardWaitBlit();
		*savewordpointer = saveword;
	}
	
	savewordpointer = (WORD *)(frontbuffer + y * BITMAPBYTESPERROW + (x / 8));
	saveword = *savewordpointer;

	DrawBlock(x,y,mapx,mapy);
	
	previous_direction = DIRECTION_LEFT;
}

static void ScrollRight(void)
{
	WORD mapx,mapy,x,y;

	if (mapposx >= (mapwidth * BLOCKWIDTH - SCREENWIDTH - BLOCKWIDTH)) return;
	
	mapx = mapposx / BLOCKWIDTH + BITMAPBLOCKSPERROW;
	mapy = mapposx & (NUMSTEPS - 1);
	
	x = BITMAPWIDTH + ROUND2BLOCKWIDTH(videoposx);
	y = mapy * BLOCKPLANELINES;
	
	if (previous_direction == DIRECTION_LEFT)
	{
		HardWaitBlit();
		*savewordpointer = saveword;
	}
	
	savewordpointer = (WORD *)(frontbuffer + (y + BLOCKPLANELINES - 1) * BITMAPBYTESPERROW + (x / 8));
	saveword = *savewordpointer;

	DrawBlock(x,y,mapx,mapy);

	mapposx++;
	videoposx = mapposx;
	
	previous_direction = DIRECTION_RIGHT;
}

static void CheckJoyScroll(void)
{
	WORD i,count;
	
	if (JoyFire()) count = 8; else count = 1;

	if (JoyLeft())
	{
		for(i = 0;i < count;i++)
		{
			ScrollLeft();
		}
	}
	
	if (JoyRight())
	{
		for(i = 0;i < count;i++)
		{
			ScrollRight();
		}
	}
}

static void UpdateCopperlist(void)
{
	ULONG pl;
	WORD xpos,planeaddx,scroll,i;
	WORD *wp;
	
	i = fetchinfo[option_fetchmode].scrollpixels;

	xpos = videoposx + i - 1;

	planeaddx = (xpos / i) * (i / 8);
	i = (i - 1) - (xpos & (i - 1));
	
	scroll = (i & 15) * 0x11;
	if (i & 16) scroll |= (0x400 + 0x4000);
	if (i & 32) scroll |= (0x800 + 0x8000);
	
	// set scroll register in BPLCON1
	
	CopBPLCON1[1] = scroll;

	// set plane pointers
	
	wp = CopPLANE1H;

	for(i = 0;i < BLOCKSDEPTH;i++)
	{
		pl = ((ULONG)ScreenBitmap->Planes[i]) + planeaddx;
		
		wp[1] = (WORD)(pl >> 16);
		wp[3] = (WORD)(pl & 0xFFFF);
		
		wp += 4;
	}
	
}

static void ShowWhatCopperWouldDo(void)
{
	WORD x;
	
	x = (videoposx+16) % BITMAPWIDTH;

	SetWriteMask(ScreenRastPort,1);
	SetAPen(ScreenRastPort,0);
	RectFill(ScreenRastPort,0,bitmapheight - 3,BITMAPWIDTH-1,bitmapheight - 3);
	SetAPen(ScreenRastPort,1);

	if (x <= EXTRAWIDTH)
	{
		RectFill(ScreenRastPort,x,bitmapheight - 3,x+SCREENWIDTH-1,bitmapheight - 3);
	} else {
		RectFill(ScreenRastPort,x,bitmapheight - 3,BITMAPWIDTH-1,bitmapheight - 3);
		RectFill(ScreenRastPort,0,bitmapheight - 3,x - EXTRAWIDTH,bitmapheight - 3);		
	}
}

static void MainLoop(void)
{
	if (!option_how)
	{
		HardWaitBlit();
		WaitVBL();

		// activate copperlist
		custom->copjmp2 = 0;
	}
	
	while (!LMBDown())
	{
		if (!option_how)
		{
			WaitVBeam(199);
			WaitVBeam(200);
		} else {
			Delay(1);
		}
		
		if (option_speed) *(WORD *)0xdff180 = 0xFF0;

		CheckJoyScroll();

		if (option_speed) *(WORD *)0xdff180 = 0xF00;

		if (!option_how)
		{
			UpdateCopperlist();
		} else {
			ShowWhatCopperWouldDo();
		}

	}
}

/********************* MAIN *************************/

void main(void)
{
	OpenLibs();
	GetArguments();
	OpenMap();
	OpenBlocks();
	OpenDisplay();

	if (!option_how)
	{
		Delay(2*50);
		KillSystem();
		InitCopperlist();
	}
	FillScreen();
	
	MainLoop();
	
	if (!option_how)
	{
		ActivateSystem();
	}

	Cleanup(0);	
}

