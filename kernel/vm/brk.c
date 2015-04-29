/******************************************************************************/
/* Important Spring 2015 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "globals.h"
#include "errno.h"
#include "util/debug.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/mman.h"

#include "vm/mmap.h"
#include "vm/vmmap.h"

#include "proc/proc.h"

/*
 * This function implements the brk(2) system call.
 *
 * This routine manages the calling process's "break" -- the ending address
 * of the process's "dynamic" region (often also referred to as the "heap").
 * The current value of a process's break is maintained in the 'p_brk' member
 * of the proc_t structure that represents the process in question.
 *
 * The 'p_brk' and 'p_start_brk' members of a proc_t struct are initialized
 * by the loader. 'p_start_brk' is subsequently never modified; it always
 * holds the initial value of the break. Note that the starting break is
 * not necessarily page aligned!
 *
 * 'p_start_brk' is the lower limit of 'p_brk' (that is, setting the break
 * to any value less than 'p_start_brk' should be disallowed).
 *
 * The upper limit of 'p_brk' is defined by the minimum of (1) the
 * starting address of the next occuring mapping or (2) USER_MEM_HIGH.
 * That is, growth of the process break is limited only in that it cannot
 * overlap with/expand into an existing mapping or beyond the region of
 * the address space allocated for use by userland. (note the presence of
 * the 'vmmap_is_range_empty' function).
 *
 * The dynamic region should always be represented by at most ONE vmarea.
 * Note that vmareas only have page granularity, you will need to take this
 * into account when deciding how to set the mappings if p_brk or p_start_brk
 * is not page aligned.
 *
 * You are guaranteed that the process data/bss region is non-empty.
 * That is, if the starting brk is not page-aligned, its page has
 * read/write permissions.
 *
 * If addr is NULL, you should NOT fail as the man page says. Instead,
 * "return" the current break. We use this to implement sbrk(0) without writing
 * a separate syscall. Look in user/libc/syscall.c if you're curious.
 *
 * Also, despite the statement on the manpage, you MUST support combined use
 * of brk and mmap in the same process.
 *
 * Note that this function "returns" the new break through the "ret" argument.
 * Return 0 on success, -errno on failure.
 */
int
do_brk(void *addr, void **ret)
{
       /* NOT_YET_IMPLEMENTED("VM: do_brk");
        return 0;*/
       /*error handling*/
	if (addr == NULL) {
		*ret = curproc->p_brk;
		return 0;
	}
	/*error handling*/
	if ((addr < curproc->p_start_brk) || (addr > USER_MEM_HIGH)) {
		return -ENOMEM;
	}
	/*quickly check if addr is inside a heap area*/
	/*	vmarea_t * new_area = vmmap_lookup(curproc->p_vmmap, ADDR_TO_PN(curproc->p_brk));
	 if (new_area == NULL) {This should be not be NULL
	 return -1;  what should we return as error
	 }
	 if (curproc->p_start_brk < addr && addr < curproc->p_brk) {
	 curproc->p_brk = PAGE_ALIGN_UP(addr);
	 new_area->vma_end = ADDR_TO_PN(PAGE_ALIGN_UP(addr));
	 *ret = curproc->p_brk;
	 return 0;
	 }*/

	/*find the end of bss region:*/
	vmarea_t * bss_area = vmmap_lookup(curproc->p_vmmap,
			ADDR_TO_PN(curproc->p_start_brk));
	/*end vfn of the bss area*/
	uint32_t bss_end = bss_area->vma_end;
	void* bss_end_addr = PN_TO_ADDR(bss_end);
	uint32_t addr_vfn = ADDR_TO_PN(addr);
	uint32_t gap = addr_vfn - bss_end; /*number of pages b-ween bss_end and addr*/
	/*check if we already have a heap vmarea:*/
	if (vmmap_is_range_empty(curproc->p_vmmap, bss_end, gap)) {
		/* heap vmarea doesn't exist, allocate a new one*/
		uint32_t lopage = bss_end;
		uint32_t numpages = ADDR_TO_PN(PAGE_ALIGN_UP(addr)) - lopage;
		vmmap_map(curproc->p_vmmap, NULL, lopage, numpages,
		PROT_READ | PROT_WRITE, MAP_PRIVATE, 0, VMMAP_DIR_HILO,
		NULL);
		curproc->p_brk = addr;
		*ret = curproc->p_brk;
		return 0;

	} else { /*heap vmarea already exists, need to find it*/
		vmarea_t * heap_area = vmmap_lookup(curproc->p_vmmap, bss_end); /*heap area must be right after bss, and end of bss area is included*/
		/*now, there're several cases*/
		/*case 1 - new addr is > curproc->p_brk*/
		if (addr > curproc->p_brk) {
			heap_area->vma_end = ADDR_TO_PN(PAGE_ALIGN_UP(addr));
			curproc->p_brk = addr;
			*ret = curproc->p_brk;
			return 0;
		}
        /*	uint32_t end_vfn = ADDR_TO_PN(PAGE_ALIGN_UP(addr));
        	if(end_vfn == new_area->vma_end) { if page-aligned vfn of a new start equals the end of new_area, increment it
        		end_vfn++;
        	}
        	new_area->vma_end = end_vfn;
        	curproc->p_brk = PAGE_ALIGN_UP(addr);
        	*ret = curproc->p_brk;*/
        /*uint32_t start_vfn = ADDR_TO_PN(PAGE_ALIGN_DOWN(curproc->p_brk));*/
        	/*Check iff the range is empty
        	int range_ret = vmmap_is_range_empty(curproc->p_vmmap, start_vfn, start_vfn-end_vfn);
        	if (range_ret){
        		curproc->p_brk = addr;
        		new_area->vma_end = ADDR_TO_PN(addr);
        		*ret = curproc->p_brk;
        	}
        	else{There is a mapping, so find the vma_area n set the end to the start of it
        		vmarea_t * area = vmmap_lookup(curproc->p_vmmap, ADDR_TO_PN(addr));
        		curproc->p_brk = PN_TO_ADDR(area->vma_start);
        		new_area->vma_end = area->vma_start;
        		ret = &curproc->p_brk;
        	}*/

        	return 0;
}
