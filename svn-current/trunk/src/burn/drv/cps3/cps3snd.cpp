#include "cps3.h"

#define CPS3_VOICES		16

#define CPS3_SND_INT_RATE		(nBurnFPS / 100)
#define CPS3_SND_RATE			(42954500 / 3 / 384)
#define CPS3_SND_BUFFER_SIZE	(CPS3_SND_RATE / CPS3_SND_INT_RATE)
#define CPS3_SND_LINEAR_SHIFT	12

typedef struct
{
	UINT16 regs[16];
	UINT32 pos;
	UINT16 frac;
} cps3_voice;

typedef struct
{
	cps3_voice voice[CPS3_VOICES];
	UINT16 key;

	UINT8 * rombase;
	UINT32 delta;
	
	double gain[2];
	INT32 output_dir[2];

} cps3snd_chip;

static cps3snd_chip * chip;

UINT8 __fastcall cps3SndReadByte(UINT32 addr)
{
	addr &= 0x000003ff;
	return 0;
}

UINT16 __fastcall cps3SndReadWord(UINT32 addr)
{
	addr &= 0x000003ff;
	if (addr < 0x200)
		return chip->voice[addr >> 5].regs[(addr>>1) & 0xf];
	if (addr == 0x200)
		return chip->key;
	return 0;
}

UINT32 __fastcall cps3SndReadLong(UINT32 addr)
{
	addr &= 0x000003ff;
	return 0;
}

void __fastcall cps3SndWriteByte(UINT32 addr, UINT8 data)
{
	addr &= 0x000003ff;
}

void __fastcall cps3SndWriteWord(UINT32 addr, UINT16 data)
{
	addr &= 0x000003ff;
	
	if (addr < 0x200)
		chip->voice[addr >> 5].regs[(addr>>1) & 0xf] = data;
	else
	if (addr == 0x200)
	{
		UINT16 key = data;
		for (INT32 i = 0; i < CPS3_VOICES; i++)
      {
			// Key off -> Key on
			if ((key & (1 << i)) && !(chip->key & (1 << i)))
         {
				chip->voice[i].frac = 0;
				chip->voice[i].pos = 0;
			}
		}
		chip->key = key;
	}
}

void __fastcall cps3SndWriteLong(UINT32 addr, UINT32 data) { }

INT32 cps3SndInit(UINT8 * sndrom)
{
	chip = (cps3snd_chip *)BurnMalloc( sizeof(cps3snd_chip) );
	if ( chip )
   {
		memset( chip, 0, sizeof(cps3snd_chip) );
		chip->rombase = sndrom;
		
		/* 
		 * CPS-3 Sound chip clock: 42954500 / 3 / 384 = 37286.89
		 * Sound interupt 80Hz 
		 */
		
		if (nBurnSoundRate)
			chip->delta = (CPS3_SND_BUFFER_SIZE << CPS3_SND_LINEAR_SHIFT) / nBurnSoundLen;
		
		chip->gain[BURN_SND_CPS3SND_ROUTE_1] = 1.00;
		chip->gain[BURN_SND_CPS3SND_ROUTE_2] = 1.00;
		chip->output_dir[BURN_SND_CPS3SND_ROUTE_1] = BURN_SND_ROUTE_LEFT;
		chip->output_dir[BURN_SND_CPS3SND_ROUTE_2] = BURN_SND_ROUTE_RIGHT;
		
		return 0;
	}
	return 1;
}

void cps3SndSetRoute(INT32 nIndex, double nVolume, INT32 nRouteDir)
{
	chip->gain[nIndex] = nVolume;
	chip->output_dir[nIndex] = nRouteDir;
}

void cps3SndExit(void) { BurnFree(chip); }

void cps3SndUpdate(void)
{
	if (!pBurnSoundOut)
		return;	
	
	memset(pBurnSoundOut, 0, nBurnSoundLen * 2 * sizeof(INT16));
	INT8 * base = (INT8 *)chip->rombase;
	cps3_voice *vptr = &chip->voice[0];

	for(INT32 i=0; i<CPS3_VOICES; i++, vptr++)
   {
      if (chip->key & (1 << i))
      {
         UINT32 start   = ((vptr->regs[ 3] << 16) | vptr->regs[ 2]) - 0x400000;
         UINT32 end     = ((vptr->regs[11] << 16) | vptr->regs[10]) - 0x400000;
         UINT32 loop    = ((vptr->regs[ 9] << 16) | vptr->regs[ 7]) - 0x400000;
         UINT32 step    = ( vptr->regs[ 6] * chip->delta ) >> CPS3_SND_LINEAR_SHIFT;

         INT32 vol_l    = (INT16)vptr->regs[15];
         INT32 vol_r    = (INT16)vptr->regs[14];

         UINT32 pos     = vptr->pos;
         UINT32 frac    = vptr->frac;

         /* Go through the buffer and add voice contributions */
         INT16 * buffer = (INT16 *)pBurnSoundOut;

         for (INT32 j=0; j<nBurnSoundLen; j++)
         {
            INT32 sample;

            pos += (frac >> 12);
            frac &= 0xfff;

            if (start + pos >= end)
            {
               if (vptr->regs[5])
                  pos = loop - start;
               else
               {
                  chip->key &= ~(1 << i);
                  break;
               }
            }

            // 8bit sample store with 16bit bigend ???
            sample = base[(start + pos) ^ 1];
            frac += step;

            INT32 nLeftSample = 0, nRightSample = 0;

            if ((chip->output_dir[BURN_SND_CPS3SND_ROUTE_1] & BURN_SND_ROUTE_LEFT) == BURN_SND_ROUTE_LEFT)
               nLeftSample += (INT32)(((sample * vol_l) >> 8) * chip->gain[BURN_SND_CPS3SND_ROUTE_1]);
            if ((chip->output_dir[BURN_SND_CPS3SND_ROUTE_1] & BURN_SND_ROUTE_RIGHT) == BURN_SND_ROUTE_RIGHT)
               nRightSample += (INT32)(((sample * vol_l) >> 8) * chip->gain[BURN_SND_CPS3SND_ROUTE_1]);

            if ((chip->output_dir[BURN_SND_CPS3SND_ROUTE_2] & BURN_SND_ROUTE_LEFT) == BURN_SND_ROUTE_LEFT)
               nLeftSample += (INT32)(((sample * vol_r) >> 8) * chip->gain[BURN_SND_CPS3SND_ROUTE_2]);
            if ((chip->output_dir[BURN_SND_CPS3SND_ROUTE_2] & BURN_SND_ROUTE_RIGHT) == BURN_SND_ROUTE_RIGHT)
               nRightSample += (INT32)(((sample * vol_r) >> 8) * chip->gain[BURN_SND_CPS3SND_ROUTE_2]);

            nLeftSample  = BURN_SND_CLIP(nLeftSample + buffer[1]);
            nRightSample = BURN_SND_CLIP(nRightSample + buffer[0]);

            buffer[0]    = nRightSample; // swapped. correct??
            buffer[1]    = nLeftSample;

            buffer      += 2;
         }


         vptr->pos = pos;
         vptr->frac = frac;
      }
   }
}

INT32 cps3SndScan(INT32 nAction)
{
	if (nAction & ACB_DRIVER_DATA)
	{
		SCAN_VAR( chip->voice );
		SCAN_VAR( chip->key );
		
	}
	return 0;
}

