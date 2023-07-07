// Cheat module

#include "burnint.h"

#define CHEAT_MAXCPU	8 /* enough? */

bool bCheatsAllowed;
CheatInfo* pCheatInfo = NULL;

static bool bCheatsEnabled = false;
static INT32 cheat_core_init_pointer = 0;

struct cheat_core
{
	cpu_core_config *cpuconfig;

	INT32 nCPU;			/* which cpu */
};

static struct cheat_core cpus[CHEAT_MAXCPU];
static cheat_core *cheat_ptr;
static cpu_core_config *cheat_subptr;

void CpuCheatRegister(INT32 nCPU, cpu_core_config *config)
{
	cheat_core *s_ptr = &cpus[cheat_core_init_pointer];

	s_ptr->cpuconfig = config;
	s_ptr->nCPU = nCPU;

	cheat_core_init_pointer++;
}

INT32 CheatUpdate()
{
	bCheatsEnabled = false;

	if (bCheatsAllowed) {
		CheatInfo* pCurrentCheat = pCheatInfo;
		CheatAddressInfo* pAddressInfo;

		while (pCurrentCheat) {
			if (pCurrentCheat->nStatus > 1) {
				pAddressInfo = pCurrentCheat->pOption[pCurrentCheat->nCurrent]->AddressInfo;
				if (pAddressInfo->nAddress) {
					bCheatsEnabled = true;
				}
			}
			pCurrentCheat = pCurrentCheat->pNext;
		}
	}

	return 0;
}

INT32 CheatEnable(INT32 nCheat, INT32 nOption)
{
	INT32 nCurrentCheat = 0;
	CheatInfo* pCurrentCheat = pCheatInfo;
	CheatAddressInfo* pAddressInfo;
	INT32 nOpenCPU = -1;

	if (!bCheatsAllowed)
		return 1;

	if (nOption >= CHEAT_MAX_OPTIONS)
		return 1;

	cheat_ptr = &cpus[0]; /* first cpu... */
	cheat_subptr = cheat_ptr->cpuconfig;

	while (pCurrentCheat && nCurrentCheat <= nCheat) {
		if (nCurrentCheat == nCheat) {

			if (nOption == -1) {
				nOption = pCurrentCheat->nDefault;
			}

			if (pCurrentCheat->nType != 1)
         {
				/* Return OK if the cheat is already active with the same option */
				if (pCurrentCheat->nCurrent == nOption)
					return 0;

				/* Deactivate old option (if any) */
				pAddressInfo = pCurrentCheat->pOption[nOption]->AddressInfo;
				while (pAddressInfo->nAddress)
            {
					if (pAddressInfo->nCPU != nOpenCPU)
               {

						if (nOpenCPU != -1)
							cheat_subptr->close();

						nOpenCPU     = pAddressInfo->nCPU;
						cheat_ptr    = &cpus[nOpenCPU];
						cheat_subptr = cheat_ptr->cpuconfig;
						cheat_subptr->open(cheat_ptr->nCPU);
					}

					/* Write back original values to memory */
					cheat_subptr->write(pAddressInfo->nAddress,
                     pAddressInfo->nOriginalValue);
					pAddressInfo++;
				}
			}

			/* Activate new option */
			pAddressInfo = pCurrentCheat->pOption[nOption]->AddressInfo;
			while (pAddressInfo->nAddress)
         {
				if (pAddressInfo->nCPU != nOpenCPU)
            {
					if (nOpenCPU != -1)
						cheat_subptr->close();

					nOpenCPU = pAddressInfo->nCPU;
					cheat_ptr = &cpus[nOpenCPU];
					cheat_subptr = cheat_ptr->cpuconfig;
					cheat_subptr->open(cheat_ptr->nCPU);
				}

				/* Copy the original values */
				pAddressInfo->nOriginalValue = cheat_subptr->read(pAddressInfo->nAddress);

				if (pCurrentCheat->nType != 0)
            {
					if (pAddressInfo->nCPU != nOpenCPU)
               {
						if (nOpenCPU != -1)
							cheat_subptr->close();

						nOpenCPU     = pAddressInfo->nCPU;
						cheat_ptr    = &cpus[nOpenCPU];
						cheat_subptr = cheat_ptr->cpuconfig;
						cheat_subptr->open(cheat_ptr->nCPU);
					}

					/* Activate the cheat */
					cheat_subptr->write(pAddressInfo->nAddress, pAddressInfo->nValue);
				}

				pAddressInfo++;
			}

			/* Set cheat status and active option */
			if (pCurrentCheat->nType != 1)
				pCurrentCheat->nCurrent = nOption;
			if (pCurrentCheat->nType == 0)
				pCurrentCheat->nStatus = 2;
			if (pCurrentCheat->nType == 2)
				pCurrentCheat->nStatus = 1;

			break;
		}
		pCurrentCheat = pCurrentCheat->pNext;
		nCurrentCheat++;
	}

	if (nOpenCPU != -1)
		cheat_subptr->close();

	CheatUpdate();

	if (nCurrentCheat == nCheat && pCurrentCheat)
		return 0;

	return 1;
}

INT32 CheatApply(void)
{
	if (!bCheatsEnabled)
		return 0;

	INT32 nOpenCPU = -1;

	CheatInfo* pCurrentCheat = pCheatInfo;
	CheatAddressInfo* pAddressInfo;
	while (pCurrentCheat)
   {
		if (pCurrentCheat->nStatus > 1)
      {
			pAddressInfo = pCurrentCheat->pOption[pCurrentCheat->nCurrent]->AddressInfo;
			while (pAddressInfo->nAddress)
         {

				if (pAddressInfo->nCPU != nOpenCPU)
            {
					if (nOpenCPU != -1)
						cheat_subptr->close();

					nOpenCPU     = pAddressInfo->nCPU;
					cheat_ptr    = &cpus[nOpenCPU];
					cheat_subptr = cheat_ptr->cpuconfig;
					cheat_subptr->open(cheat_ptr->nCPU);
				}

				cheat_subptr->write(pAddressInfo->nAddress, pAddressInfo->nValue);
				pAddressInfo++;
			}
		}
		pCurrentCheat = pCurrentCheat->pNext;
	}

	if (nOpenCPU != -1)
		cheat_subptr->close();

	return 0;
}

INT32 CheatInit(void)
{
	CheatExit();

	bCheatsEnabled = false;

	return 0;
}

void CheatExit(void)
{
	if (pCheatInfo)
   {
      CheatInfo* pCurrentCheat = pCheatInfo;
      CheatInfo* pNextCheat;

      do {
         INT32 i;
         pNextCheat = pCurrentCheat->pNext;
         for (i = 0; i < CHEAT_MAX_OPTIONS; i++)
         {
            if (pCurrentCheat->pOption[i])
               free(pCurrentCheat->pOption[i]);
         }
         if (pCurrentCheat)
            free(pCurrentCheat);
      } while ((pCurrentCheat = pNextCheat) != 0);
   }

	memset (cpus, 0, sizeof(cheat_core));

	cheat_core_init_pointer = 0;

	pCheatInfo = NULL;
	
	CheatSearchInitCallbackFunction = NULL;
}

/* Cheat search */

static UINT8 *MemoryValues = NULL;
static UINT8 *MemoryStatus = NULL;
static UINT32 nMemorySize  = 0;
CheatSearchInitCallback CheatSearchInitCallbackFunction = NULL;

#define NOT_IN_RESULTS	0
#define IN_RESULTS	1

UINT32 CheatSearchShowResultAddresses[CHEATSEARCH_SHOWRESULTS];
UINT32 CheatSearchShowResultValues[CHEATSEARCH_SHOWRESULTS];

INT32 CheatSearchInit(void) { return 1; }

void CheatSearchExit(void)
{
	if (MemoryValues)
   {
		free(MemoryValues);
		MemoryValues = NULL;
	}
	if (MemoryStatus)
   {
		free(MemoryStatus);
		MemoryStatus = NULL;
	}
	
	nMemorySize = 0;
	
	memset(CheatSearchShowResultAddresses, 0, CHEATSEARCH_SHOWRESULTS);
	memset(CheatSearchShowResultValues, 0, CHEATSEARCH_SHOWRESULTS);
}

void CheatSearchStart(void)
{
	UINT32 nAddress;
	
	INT32 nActiveCPU = 0;
	cheat_ptr = &cpus[nActiveCPU];
	cheat_subptr = cheat_ptr->cpuconfig;
	cheat_subptr->open(cheat_ptr->nCPU);

	nActiveCPU = cheat_subptr->active();
	if (nActiveCPU >= 0) cheat_subptr->close();
	cheat_subptr->open(cheat_ptr->nCPU);
	nMemorySize = cheat_subptr->nMemorySize;

	MemoryValues = (UINT8*)malloc(nMemorySize);
	MemoryStatus = (UINT8*)malloc(nMemorySize);
	
	memset(MemoryStatus, IN_RESULTS, nMemorySize);
	
	if (CheatSearchInitCallbackFunction) CheatSearchInitCallbackFunction();

	for (nAddress = 0; nAddress < nMemorySize; nAddress++) {
		if (MemoryStatus[nAddress] == NOT_IN_RESULTS) continue;
		MemoryValues[nAddress] = cheat_subptr->read(nAddress);
	}
	
	cheat_subptr->close();
	if (nActiveCPU >= 0) cheat_subptr->open(nActiveCPU);
}

static void CheatSearchGetResults(void)
{
	UINT32 nAddress;
	UINT32 nResultsPos = 0;
	
	memset(CheatSearchShowResultAddresses, 0, CHEATSEARCH_SHOWRESULTS);
	memset(CheatSearchShowResultValues, 0, CHEATSEARCH_SHOWRESULTS);
	
	for (nAddress = 0; nAddress < nMemorySize; nAddress++)
   {		
		if (MemoryStatus[nAddress] == IN_RESULTS)
      {
			CheatSearchShowResultAddresses[nResultsPos] = nAddress;
			CheatSearchShowResultValues[nResultsPos] = MemoryValues[nAddress];
			nResultsPos++;
		}
	}
}

UINT32 CheatSearchValueNoChange(void)
{
	UINT32 nAddress;
	UINT32 nMatchedAddresses = 0;
	INT32 nActiveCPU = cheat_subptr->active();
	if (nActiveCPU >= 0) cheat_subptr->close();
	cheat_subptr->open(0);
	
	for (nAddress = 0; nAddress < nMemorySize; nAddress++)
   {
		if (MemoryStatus[nAddress] == NOT_IN_RESULTS)
         continue;
		if (cheat_subptr->read(nAddress) == MemoryValues[nAddress])
      {
			MemoryValues[nAddress] = cheat_subptr->read(nAddress);
			nMatchedAddresses++;
		}
      else
			MemoryStatus[nAddress] = NOT_IN_RESULTS;
	}

	cheat_subptr->close();
	if (nActiveCPU >= 0) cheat_subptr->open(nActiveCPU);
	
	if (nMatchedAddresses <= CHEATSEARCH_SHOWRESULTS) CheatSearchGetResults();
	
	return nMatchedAddresses;
}

UINT32 CheatSearchValueChange()
{
	UINT32 nMatchedAddresses = 0;
	UINT32 nAddress;
	
	INT32 nActiveCPU = 0;
	
	nActiveCPU = cheat_subptr->active();
	if (nActiveCPU >= 0)
      cheat_subptr->close();
	cheat_subptr->open(0);
	
	for (nAddress = 0; nAddress < nMemorySize; nAddress++)
   {
		if (MemoryStatus[nAddress] == NOT_IN_RESULTS)
         continue;
		if (cheat_subptr->read(nAddress) != MemoryValues[nAddress])
      {
			MemoryValues[nAddress] = cheat_subptr->read(nAddress);
			nMatchedAddresses++;
		}
      else
			MemoryStatus[nAddress] = NOT_IN_RESULTS;
	}
	
	cheat_subptr->close();
	if (nActiveCPU >= 0)
      cheat_subptr->open(nActiveCPU);
	if (nMatchedAddresses <= CHEATSEARCH_SHOWRESULTS)
      CheatSearchGetResults();
	
	return nMatchedAddresses;
}

UINT32 CheatSearchValueDecreased(void)
{
	UINT32 nMatchedAddresses = 0;
	UINT32 nAddress;
	INT32 nActiveCPU = cheat_subptr->active();
	if (nActiveCPU >= 0)
      cheat_subptr->close();
	cheat_subptr->open(0);

	for (nAddress = 0; nAddress < nMemorySize; nAddress++)
   {
		if (MemoryStatus[nAddress] == NOT_IN_RESULTS) continue;
		if (cheat_subptr->read(nAddress) < MemoryValues[nAddress])
      {
			MemoryValues[nAddress] = cheat_subptr->read(nAddress);
			nMatchedAddresses++;
		}
      else
			MemoryStatus[nAddress] = NOT_IN_RESULTS;
	}

	cheat_subptr->close();
	if (nActiveCPU >= 0)
      cheat_subptr->open(nActiveCPU);
	
	if (nMatchedAddresses <= CHEATSEARCH_SHOWRESULTS)
      CheatSearchGetResults();
	
	return nMatchedAddresses;
}

UINT32 CheatSearchValueIncreased(void)
{
	UINT32 nMatchedAddresses = 0;
	UINT32 nAddress;
	INT32 nActiveCPU = cheat_subptr->active();
	if (nActiveCPU >= 0) cheat_subptr->close();
	cheat_subptr->open(0);

	for (nAddress = 0; nAddress < nMemorySize; nAddress++)
   {
		if (MemoryStatus[nAddress] == NOT_IN_RESULTS) continue;
		if (cheat_subptr->read(nAddress) > MemoryValues[nAddress])
      {
			MemoryValues[nAddress] = cheat_subptr->read(nAddress);
			nMatchedAddresses++;
		}
      else
			MemoryStatus[nAddress] = NOT_IN_RESULTS;
	}
	
	cheat_subptr->close();
	if (nActiveCPU >= 0) cheat_subptr->open(nActiveCPU);
	
	if (nMatchedAddresses <= CHEATSEARCH_SHOWRESULTS) CheatSearchGetResults();
	
	return nMatchedAddresses;
}

void CheatSearchExcludeAddressRange(UINT32 nStart, UINT32 nEnd)
{
   UINT32 nAddress;
	for (nAddress = nStart; nAddress <= nEnd; nAddress++)
		MemoryStatus[nAddress] = NOT_IN_RESULTS;
}

#undef NOT_IN_RESULTS
#undef IN_RESULTS
