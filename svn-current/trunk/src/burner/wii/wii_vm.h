/* Copyright 2013 tueidj All Rights Reserved
 * This code may not be used in any project
 * without explicit permission from the author.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define KB (1024)
#define MB (1024*1024)
#define ROMCACHE_SIZE (1*MB)

void* VM_Init(size_t VMSize, size_t MEMSize);
void VM_Deinit(void);

// clears entire VM range to zero, unlocks any locked pages
void VM_InvalidateAll(void);

#ifdef __cplusplus
}
#endif
