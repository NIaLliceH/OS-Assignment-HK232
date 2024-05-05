#ifndef TLBCACHE_H
#define TLBCACHE_H

#include "mm.h"

// Forward declarations for functions
int tlb_cache_read(struct memphy_struct *tlb, int pid, int pgnum, int* value);
int tlb_cache_write(struct memphy_struct *tlb, int pid, int pgnum, int value);
int tlb_cache_invalidate(struct memphy_struct *tlb, int pid, int pgnum);
int TLBMEMPHY_dump(struct memphy_struct *mp);
int init_tlbmemphy(struct memphy_struct *mp, int max_size);

//TLB FEATURES:
//FULLY-ASSOC
//RANDOM REPLACEMENT

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



#endif // TLBCACHE_H
