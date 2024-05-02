/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef MM_TLB
/*
 * Memory physical based TLB Cache
 * TLB cache module tlb/tlbcache.c
 *
 * TLB cache is physically memory phy
 * supports random access 
 * and runs at high speed
 */


#include "mm.h"
#include <stdlib.h>



#define N_ASSOC 4 //N-way associative

/* TLBEntry BIT */
#define TLB_ENTRY_PRESENT_MASK BIT(31) 

// #define PAGING_PTE_SWAPPED_MASK BIT(30)
// #define PAGING_PTE_RESERVE_MASK BIT(29)
// #define PAGING_PTE_DIRTY_MASK BIT(28)
// #define PAGING_PTE_EMPTY01_MASK BIT(14)
// #define PAGING_PTE_EMPTY02_MASK BIT(13)

#define init_tlbcache(mp,sz,...) init_memphy(mp, sz, (1, ##__VA_ARGS__))

static uint16_t time = 0;
//Entry 80 bits => 10 bytes
struct tlbEntry {
   int valid; // 1 bit
   int pid; // 32 bits
   int time; // 80 - sum = 21 bits
   
   // int pgn; // 14 bits
   //=> tag + setOffset
   int tag; //12 bits
   int setOffset; //2 bits

   int frmnum; // 12 bits
};

#define STR_INTVL 10



/*
 *  tlb_cache_read read TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */
//equivalent of pg_getpage
int tlb_cache_read(struct memphy_struct * tlb, int pid, int pgnum, int* value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
   int storageMaxSize = tlb->maxsz / STR_INTVL;
   int tag = pgnum >> N_ASSOC;
   int setOffset = (pgnum & GENMASK(N_ASSOC - 1, 0));


   ///
   return 0;
}

/*
 *  tlb_cache_write write TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */
//equivalent of pg_setpage
int tlb_cache_write(struct memphy_struct *mp, int pid, int pgnum, BYTE value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
   return 0;
}

#pragma region UNUSED

/*
 *  TLBMEMPHY_read natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int TLBMEMPHY_read(struct memphy_struct * mp, int addr, BYTE *value)
{
   if (mp == NULL)
     return -1;

   /* TLB cached is random access by native */
   *value = mp->storage[addr];

   return 0;
}


/*
 *  TLBMEMPHY_write natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int TLBMEMPHY_write(struct memphy_struct * mp, int addr, BYTE data)
{
   if (mp == NULL)
     return -1;

   /* TLB cached is random access by native */
   mp->storage[addr] = data;

   return 0;
}

/*
 *  TLBMEMPHY_format natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 */

#pragma endregion

int TLBMEMPHY_dump(struct memphy_struct * mp)
{
   /*TODO dump memphy contnt mp->storage 
    *     for tracing the memory content
    */
   int i;
	for (i = 0; i < mp->maxsz; i++) {
      printf("%02x ", mp->storage[i]);
	}
   ///

   return 0;
}


/*
 *  Init TLBMEMPHY struct
 */
int init_tlbmemphy(struct memphy_struct *mp, int max_size)
{
   mp->storage = (BYTE *)malloc(max_size*sizeof(BYTE));
   mp->maxsz = max_size;

   mp->rdmflg = 1;

   return 0;
}

//#endif
