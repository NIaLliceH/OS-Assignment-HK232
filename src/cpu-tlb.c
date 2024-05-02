/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef CPU_TLB
/*
 * CPU TLB
 * TLB module cpu/cpu-tlb.c
 */
 
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

#include "cpu-tlbcache.c"
#include "mm-vm.c"

int tlb_change_all_page_tables_of(struct pcb_t *proc,  struct memphy_struct * mp)
{
  /* TODO update all page table directory info 
   *      in flush or wipe TLB (if needed)
   */
  //???
  return 0;
}

int tlb_flush_tlb_of(struct pcb_t *proc, struct memphy_struct * mp)
{
  /* TODO flush tlb cached*/
  tlb_cache_invalidate(proc->tlb, proc->pid, -1);
  return 0;
}

/*tlballoc - CPU TLB-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr;

  /* By default using vmaid = 0 */
  if (__alloc(proc, 0, reg_index, size, &addr) != 0){
    return -1;
  }

  int pgn = PAGING_PGN(addr);
  int frmnum = -1;
  if (pg_getpage(proc->mm, pgn, &frmnum, proc) != 0)
    return -1;

  /* TODO update TLB CACHED frame num of the new allocated page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  if (tlb_cache_write(proc->tlb, proc->pid, pgn, frmnum) != 0)
    return -1;

  return 0;
}

/*pgfree - CPU TLB-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlbfree_data(struct pcb_t *proc, uint32_t reg_index)
{


  struct vm_rg_struct *currg = get_symrg_byid(proc->mm, reg_index);
  struct vm_area_struct *cur_vma = get_vma_by_num(proc->mm, 0);
  if(currg == NULL || cur_vma == NULL) /* Invalid memory identify */
	  return -1;

  //CPU address calculate
  int addr = currg->rg_start;
  int pgn =  PAGING_PGN(addr);

  /* TODO update TLB CACHED frame num of freed page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  tlb_cache_invalidate(proc->tlb, proc->pid, pgn);

  __free(proc, 0, reg_index);

  return 0;
}


/*tlbread - CPU TLB-based read a region memory
 *@proc: Process executing the instruction
 *@source: index of source register
 *@offset: source address = [source] + [offset]
 *@destination: destination storage
 */
int tlbread(struct pcb_t * proc, uint32_t source,
            uint32_t offset, 	uint32_t destination) 
{
  BYTE data;
  int frmnum = -1;
  /* TODO retrieve TLB CACHED frame num of accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  /* frmnum is return value of tlb_cache_read/write value*/

  ///get VM area and current region
  struct vm_rg_struct *currg = get_symrg_byid(proc->mm, source);
  struct vm_area_struct *cur_vma = get_vma_by_num(proc->mm, 0);
  if(currg == NULL || cur_vma == NULL) /* Invalid memory identify */
	  return -1;

  //CPU address calculate
  int addr = currg->rg_start + offset;
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);

  //get frmnum
  tlb_cache_read(proc->tlb, proc->pid, pgn, &frmnum);
  ///
	
#ifdef IODUMP
  if (frmnum >= 0)
    printf("TLB hit at read region=%d offset=%d\n", 
	         source, offset);
  else 
    printf("TLB miss at read region=%d offset=%d\n", 
	         source, offset);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  if (frmnum < 0)
  {
    //TLB MISS, GET DATA THROUGH PAGE TABLE
    if (pg_getpage(proc->mm, pgn, &frmnum, proc) != 0)
      return -1;

    /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
    /* by using tlb_cache_read()/tlb_cache_write()*/
    tlb_cache_write(proc->tlb, proc->pid, pgn, frmnum);
  }

  //Read from memphy
  int phyaddr = (frmnum << PAGING_ADDR_FPN_LOBIT) + off;
  MEMPHY_read(proc->mram, phyaddr, data);

  destination = (uint32_t) data;
  return 0;
}

/*tlbwrite - CPU TLB-based write a region memory
 *@proc: Process executing the instruction
 *@data: data to be wrttien into memory
 *@destination: index of destination register
 *@offset: destination address = [destination] + [offset]
 */
int tlbwrite(struct pcb_t * proc, BYTE data,
             uint32_t destination, uint32_t offset)
{
  int frmnum = -1;

  /* TODO retrieve TLB CACHED frame num of accessing page(s))*/
  /* by using tlb_cache_read()/tlb_cache_write()
  frmnum is return value of tlb_cache_read/write value*/

  ///get VM area and current region
  struct vm_rg_struct *currg = get_symrg_byid(proc->mm, destination);
  struct vm_area_struct *cur_vma = get_vma_by_num(proc->mm, 0);
  if(currg == NULL || cur_vma == NULL) /* Invalid memory identify */
	  return -1;

  //CPU address calculate
  int addr = currg->rg_start + offset;
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);

  //get frmnum
  tlb_cache_read(proc->tlb, proc->pid, pgn, &frmnum);
  ///

#ifdef IODUMP
  if (frmnum >= 0)
    printf("TLB hit at write region=%d offset=%d value=%d\n",
	          destination, offset, data);
	else
    printf("TLB miss at write region=%d offset=%d value=%d\n",
            destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  if (frmnum < 0)
  {
    //TLB MISS, GET DATA THROUGH PAGE TABLE
    if (pg_getpage(proc->mm, pgn, &frmnum, proc) != 0)
      return -1;

    /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
    /* by using tlb_cache_read()/tlb_cache_write()*/
    tlb_cache_write(proc->tlb, proc->pid, pgn, frmnum);
  }

  //Write from memphy
  int phyaddr = (frmnum << PAGING_ADDR_FPN_LOBIT) + off;
  MEMPHY_write(proc->mram, phyaddr, data);
  return 0;
}

//#endif
