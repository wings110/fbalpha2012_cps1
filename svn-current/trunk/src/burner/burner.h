// FB Alpha - Emulator for MC68000/Z80 based arcade games
//            Refer to the "license.txt" file for more info
#ifndef _BURN_H
#define _BURN_H

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>

#include "tchar.h"

// Macro to make quoted strings
#define MAKE_STRING_2(s) #s
#define MAKE_STRING(s) MAKE_STRING_2(s)

#include "burn.h"
#include "burner_libretro.h"

#if defined (INCLUDE_LIB_PNGH)
 #include "png.h"
#endif

// ---------------------------------------------------------------------------
// OS independent functionality

// gami.cpp
extern struct GameInp* GameInp;
extern UINT32 nGameInpCount;
extern UINT32 nMacroCount;
extern UINT32 nMaxMacro;

extern INT32 nAnalogSpeed;

extern INT32 nFireButtons;

extern bool bStreetFighterLayout;

INT32 GameInpInit();
INT32 GameInpExit();
INT32 GameInpBlank(INT32 bDipSwitch);
INT32 GameInpDefault();

// Player Default Controls
extern INT32 nPlayerDefaultControls[4];
extern TCHAR szPlayerDefaultIni[4][MAX_PATH];

// cong.cpp
extern const INT32 nConfigMinVersion;					// Minimum version of application for which input files are valid

// gamc.cpp
INT32 GamcMisc(struct GameInp* pgi, char* szi, INT32 nPlayer);
INT32 GamcAnalogKey(struct GameInp* pgi, char* szi, INT32 nPlayer, INT32 nSlide);
INT32 GamcAnalogJoy(struct GameInp* pgi, char* szi, INT32 nPlayer, INT32 nJoy, INT32 nSlide);
INT32 GamcPlayer(struct GameInp* pgi, char* szi, INT32 nPlayer, INT32 nDevice);
INT32 GamcPlayerHotRod(struct GameInp* pgi, char* szi, INT32 nPlayer, INT32 nFlags, INT32 nSlide);

// state.cpp
INT32 BurnStateLoad(TCHAR* szName, INT32 bAll, INT32 (*pLoadGame)());
INT32 BurnStateSave(TCHAR* szName, INT32 bAll);

// statec.cpp
INT32 BurnStateCompress(UINT8** pDef, INT32* pnDefLen, INT32 bAll);
INT32 BurnStateDecompress(UINT8* Def, INT32 nDefLen, INT32 bAll);

// zipfn.cpp
struct ZipEntry { char* szName;	UINT32 nLen; UINT32 nCrc; };

INT32 ZipOpen(char* szZip);
INT32 ZipClose();
INT32 ZipGetList(struct ZipEntry** pList, INT32* pnListCount);
INT32 ZipLoadFile(UINT8* Dest, INT32 nLen, INT32* pnWrote, INT32 nEntry);

#endif
