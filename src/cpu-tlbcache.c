/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
// #ifdef MM_TLB
/*
 * Memory physical based TLB Cache
 * TLB cache module tlb/tlbcache.c
 *
 * TLB cache is physically memory phy
 * supports random access 
 * and runs at high speed
 */

#include "mm.h"
#include "cpu-tlbcache.h"
#include <stdlib.h>
#include <stdio.h>

#define init_tlbcache(mp,sz,...) init_memphy(mp, sz, (1, ##__VA_ARGS__))

#define TLB_DBG


void print_entry(TLB_entry_t entry){
      printf("%01lld %05lld %05lld %05lld",
      TLB_VALID(entry),
      TLB_PID(entry),
      TLB_TAG(entry),
      TLB_FRMNUM(entry)
   );
}

void set_TLB_entry(TLB_entry_t *entry, int valid, int pgnum, int pid, int frmnum){
   *entry = 0;
   SET_TLB_VALID(*entry, valid);
   SET_TLB_TAG(*entry, pgnum);
   SET_TLB_PID(*entry, pid);
   SET_TLB_FRMNUM(*entry, frmnum);
}

/*
 *  tlb_cache_read read TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */
int tlb_cache_read(struct memphy_struct * tlb, int pid, int pgnum, int* frmnum)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
   //Cast storage to uint64, as each entry is 64 bits
   int storageSz = tlb->maxsz / sizeof(TLB_entry_t);

   //SIMULATE FULLY-ASSOC PARALLEL COMPARATORS BY LOOPING
   for (int index = 0; index < storageSz; index++){
      TLB_entry_t entry = 0;
      TLBMEMPHY_read(tlb, index, &entry);

      if (TLB_VALID(entry) 
      && TLB_TAG(entry) == pgnum 
      && TLB_PID(entry) == pid){
         *frmnum = TLB_FRMNUM(entry);
         return 0;
      }
   }
   
   *frmnum = -1;
   return 0;
}

/*
 *  tlb_cache_write write TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */
int tlb_cache_write(struct memphy_struct *tlb, int pid, int pgnum, int value)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
   int storageSz = tlb->maxsz / sizeof(TLB_entry_t);

   // pthread_mutex_lock(&tlb_lock);
   //PRIORITIZE FINDING EXISTING ENTRY TO UPDATE
   for (int index = 0; index < storageSz; index++){

      TLB_entry_t entry = 0;
      TLBMEMPHY_read(tlb, index, &entry);

      if (TLB_VALID(entry) 
      && TLB_TAG(entry) == pgnum 
      && TLB_PID(entry) == pid){
         //FOUND EXISTING ENTRY
         set_TLB_entry(&entry, 1, pgnum, pid, value);
         
         TLBMEMPHY_write(tlb, index, entry);

         return 0;
      }
   }

   //FIND FREE/INVALID SPACE
   for (int index = 0; index < storageSz; index++){
      TLB_entry_t entry = 0;
      TLBMEMPHY_read(tlb, index, &entry);

      if (!TLB_VALID(entry)){
         //FOUND SPACE
         set_TLB_entry(&entry, 1, pgnum, pid, value);
         TLBMEMPHY_write(tlb, index, entry);

         return 0;
      }
   }

   //FREE SPACE NOT FOUND, REPLACE RANDOM EXISTING ENTRY
   int r = rand() % storageSz;
   TLB_entry_t victimEntry = 0;
   TLBMEMPHY_read(tlb, r, &victimEntry);

   set_TLB_entry(&victimEntry, 1, pgnum, pid, value);
   TLBMEMPHY_write(tlb, r, victimEntry);
   
   // pthread_mutex_unlock(&tlb_lock);
   return 0;
}

//pgnum = -1 to invalidate all entries with pid
int tlb_cache_invalidate(struct memphy_struct *tlb, int pid, int pgnum)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */
   int storageSz = tlb->maxsz / sizeof(TLB_entry_t);

   int found = -1;
   int index;
   // pthread_mutex_lock(&tlb_lock);
   for (index = 0; index < storageSz; index++){
      TLB_entry_t entry = 0;
      TLBMEMPHY_read(tlb, index, &entry);

      if (TLB_VALID(entry) && TLB_PID(entry) == pid){
         //FOUND EXISTING ENTRY OF PID
         if (pgnum < 0 || TLB_TAG(entry) == pgnum){
            SET_TLB_VALID(entry, 0);

            TLBMEMPHY_write(tlb, index, entry);

            found = 0;
            //Return if it's not the clear all entry operation
            if (pgnum != -1) 
               return 0;
         }
      }
   }
   // pthread_mutex_unlock(&tlb_lock);

   return found;
}

/*
 *  TLBMEMPHY_read natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int TLBMEMPHY_read(struct memphy_struct * mp, int addr, TLB_entry_t *value)
{
   if (mp == NULL)
     return -1;

   /* TLB cached is random access by native */
   *value = ((TLB_entry_t *)mp->storage)[addr];

   return 0;
}

/*
 *  TLBMEMPHY_write natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int TLBMEMPHY_write(struct memphy_struct * mp, int addr, TLB_entry_t data)
{
   if (mp == NULL)
     return -1;

   /* TLB cached is random access by native */
   ((TLB_entry_t *)mp->storage)[addr] = data;

   return 0;
}

/*
 *  TLBMEMPHY_format natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 */

int TLBMEMPHY_dump(struct memphy_struct * tlb)
{
   /*TODO dump memphy contnt mp->storage 
    *     for tracing the memory content
    */
   int storageSz = tlb->maxsz / sizeof(TLB_entry_t);
   int i;

   // sprintf(stdout, "%5s %5s %5s %5s\n", 
   //    "Valid", "TAG", "PID", "FRMNUM");

	for (i = 0; i < storageSz; i++) {
      TLB_entry_t entry = 0;
      TLBMEMPHY_read(tlb, i, &entry);
      if (TLB_VALID(entry)){
         print_entry(entry);
         puts("");
      }
      // else break;
	}
   ///

   return 0;
}


/*
 *  Init TLBMEMPHY struct
 */
int init_tlbmemphy(struct memphy_struct *mp, int max_size)
{
   // pthread_mutex_init(&tlb_lock, NULL);
   mp->storage = (BYTE *)malloc(max_size*sizeof(BYTE));
   mp->maxsz = max_size;

   mp->rdmflg = 1;

   return 0;
}

//#endif
