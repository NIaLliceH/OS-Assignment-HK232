/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;

#ifdef MM_PAGING
/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
{
  struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;

  if (rg_elmt -> rg_start >= rg_elmt -> rg_end)
    return -1;

  if (rg_node != NULL)
    rg_elmt -> rg_next = rg_node;

  /* Enlist the new region */
  mm->mmap->vm_freerg_list = rg_elmt;
  
  //printf("enlist  %li %li\n",mm->mmap->vm_freerg_list -> rg_end, mm->mmap->vm_freerg_list -> rg_start);
  
  return 0;
}

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid)
{
  struct vm_area_struct *pvma= mm->mmap;

  if(mm->mmap == NULL)
    return NULL;

  int vmait = 0;
  
  while (vmait < vmaid)
  {
    if(pvma == NULL)
	  return NULL;

    vmait++;
    pvma = pvma->vm_next;
  }

  return pvma;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if(rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return NULL;

  return &mm->symrgtbl[rgid];
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *@alloc_addr: address of allocated memory region
 *
 */
int __alloc(struct pcb_t *caller, int vmaid, int rgid, int size, int *alloc_addr)
{
  pthread_mutex_lock(&mmvm_lock);
  /*Allocate at the toproof */
							  
  struct vm_rg_struct rgnode;

  int inc_sz = PAGING_PAGE_ALIGNSZ(size);

  if (get_free_vmrg_area(caller, vmaid, inc_sz, &rgnode) == 0)
  {
    caller->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;

    struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
    int inc_sz = rgnode.rg_end - rgnode.rg_start;
    int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
    int incnumpage = inc_amt / PAGING_PAGESZ;

    if (vm_map_ram(caller, rgnode.rg_start, rgnode.rg_end, rgnode.rg_start, incnumpage, newrg) < 0)
    {
      pthread_mutex_unlock(&mmvm_lock);
      return -1;
    }

    *alloc_addr = rgnode.rg_start;
		pthread_mutex_unlock(&mmvm_lock);
    return 0;
  }

  /* TODO get_free_vmrg_area FAILED handle the region management (Fig.6)*/

  /*Attempt to increate limit to get space */
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  // int inc_limit_ret
  int old_sbrk;

  old_sbrk = cur_vma->sbrk;

  /* TODO INCREASE THE LIMIT
   * inc_vma_limit(caller, vmaid, inc_sz)
   */
  inc_vma_limit(caller, vmaid, inc_sz);

  /*Successful increase limit */
  caller->mm->symrgtbl[rgid].rg_start = old_sbrk;
  caller->mm->symrgtbl[rgid].rg_end = old_sbrk + size;

  *alloc_addr = old_sbrk;

  // struct vm_area_struct *remain_rg = get_vma_by_num(caller->mm, vmaid);
  // if (old_sbrk + size < remain_rg->sbrk)
  // {
  //   struct vm_rg_struct *rg_free = malloc(sizeof(struct vm_rg_struct));
  //   rg_free->rg_start = old_sbrk + size;
  //   rg_free->rg_end = remain_rg->sbrk;
  //   enlist_vm_freerg_list(caller->mm, rg_free);
  // }

  pthread_mutex_unlock(&mmvm_lock);

  return 0;
}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int clear_pgn_node(struct pcb_t * proc , int pgn){
  struct pgn_t* prev = NULL;
  struct pgn_t* temp = proc->mm->fifo_pgn;
  if(temp==NULL) return -1;
  while(temp != NULL){
    if(temp->pgn == pgn){
      //Found the node to delete
      if(prev == NULL){
        proc->mm->fifo_pgn = temp->pg_next;
      }
      else{
        prev->pg_next = temp->pg_next;
      }
      break;
    } else {
      prev = temp;
    }
    temp = temp->pg_next;
  }

  //Free the node if it's found in the loop
  if (temp != NULL) 
    free(temp);

  return 0;
}
int __free(struct pcb_t *caller, int vmaid, int rgid)
{
 struct vm_rg_struct *rgnode = (struct vm_rg_struct *)malloc(sizeof(struct vm_rg_struct));

  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return -1;

  pthread_mutex_lock(&mmvm_lock);
  /* TODO: Manage the collect freed region to freerg_list */
  struct vm_rg_struct *temp = get_symrg_byid(caller->mm, rgid);
  if (temp->rg_end == 0)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }
  int inc_sz = temp->rg_end - temp->rg_start;
  int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage = inc_amt / PAGING_PAGESZ;
  int pgn = PAGING_PGN(temp->rg_start);

  for (int i = 0; i < incnumpage; i++)
  {
    int fpn;
    if (pg_getpage(caller->mm, pgn + i, &fpn, caller) != 0) return -1;
    MEMPHY_put_freefp(caller->mram, fpn);
    CLRBIT(caller->mm->pgd[pgn + i], PAGING_PTE_PRESENT_MASK);
    clear_pgn_node(caller, pgn+i);
  }

  rgnode->rg_start = temp->rg_start;
  rgnode->rg_end = temp->rg_end;
  rgnode->rg_next = NULL;
  temp->rg_start = 0;
  temp->rg_end = 0;
  temp->rg_next = NULL;
  /*enlist the obsoleted memory region */
  enlist_vm_freerg_list(caller->mm, rgnode);
  pthread_mutex_unlock(&mmvm_lock);
  return 0;
}

/*pgalloc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int pgalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr;

  /* By default using vmaid = 0 */
  return __alloc(proc, 0, reg_index, size, &addr);
}

/*pgfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int pgfree_data(struct pcb_t *proc, uint32_t reg_index)
{
   return __free(proc, 0, reg_index);
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  uint32_t pte = mm->pgd[pgn];

  *fpn = -1;

  //Page not allocated
  if (!PAGING_PAGE_PRESENT(pte))
    return -1;

  if (pte & PAGING_PTE_SWAPPED_MASK)
  { /* Page is not online, make it actively living */
    int vicpgn, swpfpn;
    int vicfpn;
    uint32_t vicpte;

    int swapType = pte & PAGING_PTE_SWPTYP_MASK;
    int tgtfpn = PAGING_SWP(pte); // the target frame storing our variable

    caller->active_mswp = caller->mswp[swapType];

    /* TODO: Play with your paging theory here */
    /* Find victim page */
    if (find_victim_page(caller->mm, &vicpgn) != 0)
    {
      return -1;
    }

    /* Get free frame in MEMSWP */
    if (MEMPHY_get_freefp(caller->active_mswp, &swpfpn) != 0)
    {
      return -1;
    }
    vicpte = mm->pgd[vicpgn];
    vicfpn = vicpte & PAGING_PTE_FPN_MASK;

    /* Do swap frame from MEMRAM to MEMSWP and vice versa*/
    /* Copy victim frame to swap */
    __swap_cp_page(caller->mram, vicfpn, caller->active_mswp, swpfpn);
    /* Copy target frame from swap to mem */
    __swap_cp_page(caller->active_mswp, tgtfpn, caller->mram, vicfpn);

    // print_pgtbl(caller, 0, -1);
    // printf("setaaaaaaaaaaaaaaaa%d: %d: %08x\n", vicpgn, swpfpn, mm->pgd[vicpgn]);

    pte_set_swap(&mm->pgd[vicpgn], swapType, swpfpn);

    // print_pgtbl(caller, 0, -1);

    /* Update its online status of the target page */
    pte_set_fpn(&mm->pgd[pgn], vicfpn);

    pte = mm->pgd[pgn];

    // enlist_pgn_node(&caller->mm->fifo_pgn, pgn);
  }

  *fpn = pte & PAGING_PTE_FPN_MASK;

  if (*fpn == 32){
    printf("Error\n");
  }
  return 0;
}

/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess 
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if(pg_getpage(mm, pgn, &fpn, caller) != 0) 
    return -1; /* invalid page access */

  int phyaddr = (fpn  << PAGING_ADDR_FPN_LOBIT) + off;

  MEMPHY_read(caller->mram,phyaddr, data);

  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess 
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if(pg_getpage(mm, pgn, &fpn, caller) != 0) 
    return -1; /* invalid page access */

  int phyaddr = (fpn  << PAGING_ADDR_FPN_LOBIT) + off;

  MEMPHY_write(caller->mram,phyaddr, value);

   return 0;
}

/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region 
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if( currg -> rg_start >= currg -> rg_end || currg == NULL || cur_vma == NULL) /* Invalid memory identify */
	  return -1;

  int addr = currg->rg_start + offset;
    
  if (addr > currg->rg_end){
    #ifdef TLB_DUMP
      printf("Address out of range!\n");
    #endif
    return -1;
  }

  pg_getval(caller->mm, addr, data, caller);

  return 0;
}


/*pgwrite - PAGING-based read a region memory */
int pgread(
		struct pcb_t * proc, // Process executing the instruction
		uint32_t source, // Index of source register
		uint32_t offset, // Source address = [source] + [offset]
		uint32_t destination) 
{
  BYTE data;
  int val = __read(proc, 0, source, offset, &data);

  destination = (uint32_t) data;
#ifdef IODUMP
  printf("read region=%d offset=%d value=%d\n", source, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TLB
#endif
  MEMPHY_dump(proc->mram);
#endif

  return val;
}

/*__write - write a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region 
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size 
 *
 */
int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value)
{
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);
  
  if(currg == NULL || cur_vma == NULL) /* Invalid memory identify */
	  return -1;

  int addr = currg->rg_start + offset;
    
  if (addr > currg->rg_end){
    #ifdef TLB_DUMP
      printf("Address out of range!\n");
    #endif
    return -1;
  }
  

  pg_setval(caller->mm, currg->rg_start + offset, value, caller);

  return 0;
}

/*pgwrite - PAGING-based write a region memory */
int pgwrite(
		struct pcb_t * proc, // Process executing the instruction
		BYTE data, // Data to be wrttien into memory
		uint32_t destination, // Index of destination register
		uint32_t offset)
{

  int ret = __write(proc, 0, destination, offset, data);
#ifdef IODUMP
  printf("write region=%d offset=%d value=%d\n", destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  return ret;
}


/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller)
{
  int pagenum, fpn;
  uint32_t pte;


  for(pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++)
  {
    pte= caller->mm->pgd[pagenum];

    if (!PAGING_PAGE_PRESENT(pte))
    {
      fpn = PAGING_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
    } else {
      fpn = PAGING_SWP(pte);
      MEMPHY_put_freefp(caller->active_mswp, fpn);    
    }
  }

  return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct* get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, int size, int alignedsz)
{
  struct vm_rg_struct * newrg;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  newrg = malloc(sizeof(struct vm_rg_struct));

  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = newrg->rg_start + size;

  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, int vmastart, int vmaend)
{
  //struct vm_area_struct *vma = caller->mm->mmap;

  /* TODO validate the planned memory area is not overlapped */
  if (vmastart >= vmaend)
  {
    return -1;
  }
  struct vm_area_struct *vma = caller->mm->mmap;
  struct vm_area_struct *cur_area = get_vma_by_num(caller->mm, vmaid);
  if (cur_area == NULL)
  {
    return -1;
  }
 //printf ("vmaid= %i - %p - %li - %p\n ",vmaid, vma, vma->vm_id, cur_area);
  while (vma)
  {
    if (vma != cur_area && OVERLAP(vmastart, vmaend, vma->vm_start, vma->vm_end))
    {
      return -1;
    }
    vma = vma->vm_next;
  }
  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size 
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz)
{
  struct vm_rg_struct * newrg = malloc(sizeof(struct vm_rg_struct));
  int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage =  inc_amt / PAGING_PAGESZ;
  struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  int old_end = cur_vma->vm_end;

  /*Validate overlap of obtained region */
  if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0)
    return -1; /*Overlap and failed allocation */

  /* The obtained vm area (only) 
   * now will be alloc real ram region */
  cur_vma->vm_end += inc_sz;
  cur_vma->sbrk += inc_sz;
  if (vm_map_ram(caller, area->rg_start, area->rg_end, 
                    old_end, incnumpage , newrg) < 0)
    return -1; /* Map the memory to MEMRAM */

  return 0;

}

/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
int find_victim_page(struct mm_struct *mm, int *retpgn) 
{
  struct pgn_t *pg = mm->fifo_pgn;

  /* TODO: Implement the theoretical mechanism to find the victim page */
  if (!pg)
  {
    return -1;
  }
  struct pgn_t *prev = NULL;
  while (pg->pg_next)
  {
    prev = pg;
    pg = pg->pg_next;
  }
  *retpgn = pg->pgn;
  
  if (prev) prev->pg_next = NULL;
  
  free(pg);

  // Check if the victim page is also swapped out
  uint32_t pte = mm->pgd[*retpgn];
  if (pte & PAGING_PTE_SWAPPED_MASK)
  {
    // Find another victim page recursively
    enlist_pgn_node(&mm->fifo_pgn, *retpgn);
    return find_victim_page(mm, retpgn);
  }

  return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size 
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

  if (rgit == NULL)
    return -1;

  /* Probe unintialized newrg */
  newrg->rg_start = newrg->rg_end = -1;

  /* Traverse on list of free vm region to find a fit space */
  while (rgit != NULL)
  {
    if (rgit->rg_start + size <= rgit->rg_end)
    { /* Current region has enough space */
      newrg->rg_start = rgit->rg_start;
      newrg->rg_end = rgit->rg_start + size;

      /* Update left space in chosen region */
      if (rgit->rg_start + size < rgit->rg_end)
      {
        rgit->rg_start = rgit->rg_start + size;
      }
      else
      { /*Use up all space, remove current node */
        /*Clone next rg node */
        struct vm_rg_struct *nextrg = rgit->rg_next;

        /*Cloning */
        if (nextrg != NULL)
        {
          rgit->rg_start = nextrg->rg_start;
          rgit->rg_end = nextrg->rg_end;

          rgit->rg_next = nextrg->rg_next;

          free(nextrg);
        }
        else
        { /*End of free list */
          rgit->rg_start = rgit->rg_end;	//dummy, size 0 region
          rgit->rg_next = NULL;
        }
      }
    }
    else
    {
      rgit = rgit->rg_next;	// Traverse next rg
    }
  }

 if(newrg->rg_start == -1) // new region not found
   return -1;

 return 0;
}

#endif
