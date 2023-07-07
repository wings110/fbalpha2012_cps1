// Burn - Rom Loading module
#include "burnint.h"

// Load a ROM and separate out the bytes by nGap
// Dest is the memory block to insert the ROM into
INT32 BurnLoadRom(UINT8 *Dest, INT32 i, INT32 nGap)
{
   char* RomName = ""; //add by emufan
   struct BurnRomInfo ri;
   INT32 nRet = 0, nLen = 0;
   if (!BurnExtLoadRom)
      return 1; // Load function was not defined by the application

   // Find the length of the ROM (as given by the current driver)
   ri.nType=0;
   ri.nLen=0;
   BurnDrvGetRomInfo(&ri,i);
   if (ri.nType==0)
      return 0; // Empty ROM slot - don't load anything and return success
   nLen=ri.nLen;

   BurnDrvGetRomName(&RomName, i, 0);

   if (nLen<=0)
      return 1;

   if (nGap>1)
   {
      UINT8 *pd=NULL,*pl=NULL,*LoadEnd=NULL;
      INT32 nLoadLen=0;
      // Allocate space for the file
      UINT8 *Load=(UINT8 *)malloc(nLen);
      if (Load==NULL)
         return 1;
      memset(Load,0,nLen);

      // Load in the file
      if ((nRet=BurnExtLoadRom(Load,&nLoadLen,i)) != 0)
      {
         if (Load)
            free(Load);
         Load = NULL;
         return 1;
      }

      if (nLoadLen<0) nLoadLen=0;
      if (nLoadLen>nLen) nLoadLen=nLen;

      // Loaded rom okay. Now insert into Dest
      LoadEnd=Load+nLoadLen;
      pd=Dest; pl=Load;
      // Quickly copy in the bytes with a gap of 'nGap' between each byte

      do { *pd  = *pl++; pd+=nGap; } while (pl<LoadEnd);
      if (Load)
      {
         free(Load);
         Load = NULL;
      }
   }
   else
   {
      // If no XOR, and gap of 1, just copy straight in
      if ((nRet = BurnExtLoadRom(Dest,NULL,i)) != 0)
         return 1;
   }

   return 0;
}
