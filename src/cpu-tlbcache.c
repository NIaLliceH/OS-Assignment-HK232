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
#include <stdio.h>

//TLB FEATURES:
//FULLY-ASSOC
//RANDOM REPLACEMENT
//WRITE-BACK WHEN REPLACED

/* TLB Entry BIT */
//FILLER BITS TO GET TO 64 BITS PER ENTRY
#define FREE_HIBIT 63
#define FREE_LOBIT 59

//ENTRY IS BEING USED OR NOT
#define VALID_BIT 58

//UNUSED
//FOR WRITE-BACK CHECK WHEN ENTRY IS REPLACED
// #define DIRTY_BIT 58

//FULLY-ASSOC, TAG BIT = PGN BIT = 14
#define TAG_HIBIT 57
#define TAG_LOBIT 44

//PID BIT = 32
#define PID_HIBIT 43
#define PID_LOBIT 12

//FRMNUM BIT = 12
#define FRMNUM_HIBIT 11
#define FRMNUM_LOBIT 0

//TLB Entry bit-masks
#define TLB_BITS_PER_LONG 64
#define TLB_GENMASK(h, l) \
	(((~0ULL) << (l)) & (~0ULL >> (TLB_BITS_PER_LONG  - (h) - 1)))

#define TLB_ENTRY_FREE_MASK TLB_GENMASK(FREE_HIBIT, FREE_LOBIT)
#define TLB_ENTRY_VALID_MASK BIT_ULL(VALID_BIT) 
// #define TLB_ENTRY_DIRTY_MASK BIT(DIRTY_BIT)
#define TLB_ENTRY_TAG_MASK TLB_GENMASK(TAG_HIBIT, TAG_LOBIT)
#define TLB_ENTRY_PID_MASK TLB_GENMASK(PID_HIBIT, PID_LOBIT)
#define TLB_ENTRY_FRMNUM_MASK TLB_GENMASK(FRMNUM_HIBIT, FRMNUM_LOBIT)

//COPIED FROM mm.h FOR EASIER VIEWING
#define TLB_SETVAL(v,value,mask,offst) (v=((uint64_t)v&~mask)|(((uint64_t)value<<offst)&mask))
#define TLB_GETVAL(v,mask,offst) ((v&mask)>>offst)

//TLB Entry bits extract
#define TLB_FREE(x) TLB_GETVAL(x, TLB_ENTRY_FREE_MASK, FREE_LOBIT)
#define TLB_VALID(x) TLB_GETVAL(x, TLB_ENTRY_VALID_MASK, VALID_BIT)
// #define TLB_DIRTY(x) TLB_GETVAL(x, TLB_ENTRY_DIRTY_MASK, DIRTY_BIT)
#define TLB_TAG(x) TLB_GETVAL(x, TLB_ENTRY_TAG_MASK, TAG_LOBIT)
#define TLB_PID(x) TLB_GETVAL(x, TLB_ENTRY_PID_MASK, PID_LOBIT)
#define TLB_FRMNUM(x) TLB_GETVAL(x, TLB_ENTRY_FRMNUM_MASK, FRMNUM_LOBIT)

//TLB Entry bits set
#define SET_TLB_FREE(x, value) TLB_SETVAL(x, value, TLB_ENTRY_FREE_MASK, FREE_LOBIT)
#define SET_TLB_VALID(x, value) TLB_SETVAL(x, value, TLB_ENTRY_VALID_MASK, VALID_BIT)
// #define SET_TLB_DIRTY(x, value) TLB_SETVAL(x, value, TLB_ENTRY_DIRTY_MASK, DIRTY_BIT)
#define SET_TLB_TAG(x, value) TLB_SETVAL(x, value, TLB_ENTRY_TAG_MASK, TAG_LOBIT)
#define SET_TLB_PID(x, value) TLB_SETVAL(x, value, TLB_ENTRY_PID_MASK, PID_LOBIT)
#define SET_TLB_FRMNUM(x, value) TLB_SETVAL(x, value, TLB_ENTRY_FRMNUM_MASK, FRMNUM_LOBIT)


#define init_tlbcache(mp,sz,...) init_memphy(mp, sz, (1, ##__VA_ARGS__))

//in bytes
#define ENTRY_SZ (((FREE_HIBIT - FRMNUM_LOBIT) + 1) / 8)

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
   //Cast storage to uint64, as each entry is 64 bits
   uint64_t* storage = (uint64_t*)tlb->storage;
   int storageSz = tlb->maxsz / sizeof(uint64_t);

   //SIMULATE FULLY-ASSOC PARALLEL COMPARATORS BY LOOPING
   int index = 0;
   for (; index < storageSz; index++){
      uint64_t *entry = &(storage[index]);
      if (TLB_VALID(*entry) 
      && TLB_TAG(*entry) == pgnum 
      && TLB_PID(*entry) == pid){
         *value = TLB_FRMNUM(*entry);
         return 0;
      }
   }
   
   *value = -1;
   return -1;
}

/*
 *  tlb_cache_write write TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */
//equivalent of pg_setpage

void set_TLB_entry(uint64_t *entry, int valid, int pgnum, int pid, int frmnum){
   *entry = (*entry) & 0;
   SET_TLB_VALID(*entry, valid);
   SET_TLB_TAG(*entry, pgnum);
   SET_TLB_PID(*entry, pid);
   SET_TLB_FRMNUM(*entry, frmnum);
}

int tlb_cache_write(struct memphy_struct *tlb, int pid, int pgnum, int value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
   uint64_t* storage = (uint64_t*)tlb->storage;
   int storageSz = tlb->maxsz / sizeof(uint64_t);

   int index;
   //PRIORITIZE FINDING EXISTING ENTRY TO UPDATE
   for (index = 0; index < storageSz; index++){
      uint64_t *entry = &(storage[index]);
      if (TLB_VALID(*entry) 
      && TLB_TAG(*entry) == pgnum 
      && TLB_PID(*entry) == pid){
         //FOUND EXISTING ENTRY
         set_TLB_entry(entry, 1, pgnum, pid, value);
         
         return 0;
      }
   }

   //FIND FREE/INVALID SPACE
   for (index = 0; index < storageSz; index++){
      uint64_t *entry = &(storage[index]);
      if (!TLB_VALID(*entry)){
         //FOUND SPACE
         set_TLB_entry(entry, 1, pgnum, pid, value);
         
         return 0;
      }
   }

   //FREE SPACE NOT FOUND, REPLACE EXISTING
   int r = rand() % storageSz;
   uint64_t *victimEntry = &(storage[r]);
   set_TLB_entry(victimEntry, 1, pgnum, pid, value);

   return 0;
}

//pgnum = -1 to invalidate all entries with pid
int tlb_cache_invalidate(struct memphy_struct *tlb, int pid, int pgnum)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
   uint64_t* storage = (uint64_t*)tlb->storage;
   int storageSz = tlb->maxsz / sizeof(uint64_t);

   int index;
   for (index = 0; index < storageSz; index++){
      uint64_t *entry = &(storage[index]);
      if (TLB_VALID(*entry) && TLB_PID(*entry) == pid){
         //FOUND EXISTING ENTRY OF PID
         if (pgnum < 0 || TLB_TAG(*entry) == pgnum)
            SET_TLB_VALID(*entry, 0);
      }
   }

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
