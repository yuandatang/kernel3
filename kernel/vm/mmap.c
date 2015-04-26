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
#include "types.h"

#include "mm/mm.h"
#include "mm/tlb.h"
#include "mm/mman.h"
#include "mm/page.h"

#include "proc/proc.h"

#include "util/string.h"
#include "util/debug.h"

#include "fs/vnode.h"
#include "fs/vfs.h"
#include "fs/file.h"

#include "vm/vmmap.h"
#include "vm/mmap.h"

/*
 * This function implements the mmap(2) syscall, but only
 * supports the MAP_SHARED, MAP_PRIVATE, MAP_FIXED, and
 * MAP_ANON flags.
 *
 * Add a mapping to the current process's address space.
 * You need to do some error checking; see the ERRORS section
 * of the manpage for the problems you should anticipate.
 * After error checking most of the work of this function is
 * done by vmmap_map(), but remember to clear the TLB.
 */
int
do_mmap(void *addr, size_t len, int prot, int flags,
        int fd, off_t off, void **ret)
{
        /*NOT_YET_IMPLEMENTED("VM: do_mmap");
        return -1;*/
        if(flags==MAP_PRIVATE && curproc->p_files[fd]->f_mode!=FMODE_READ)
        		return -EACCES;

        	if(flags == MAP_SHARED && prot == PROT_WRITE && curproc->p_files[fd]->f_mode != (FMODE_READ || FMODE_WRITE))
        		return -EACCES;

        	if(flags == MAP_SHARED && prot == PROT_WRITE && curproc->p_files[fd]->f_mode != FMODE_APPEND)
        		return -EACCES;

        	if((fd < 0 || fd >= NFILES || (curproc->p_files[fd] == NULL)) && flags!=MAP_ANON)
        	     return -EBADF;


        	if (len==0)
        	return -EINVAL;

        	if(PAGE_ALIGNED(addr)==0)
        	return -EINVAL;
        	/*what should be the offset value be for einval,,,,can it be 0*/

        	if ((flags!=MAP_PRIVATE) && (flags!=MAP_SHARED) && flags != MAP_FIXED && flags != MAP_ANON )
        			return -EINVAL;
        	if (flags==(MAP_PRIVATE|MAP_SHARED))
        		return -EINVAL;
        	/*what for -ENFILE,number greater than max open file limit*/
        	if(prot != PROT_EXEC)
        	     return -EPERM;

        	file_t *new_file=fget(-1);
        	if(new_file==0){
        		fput(new_file);
        		return -ENFILE;
        	}

        	/*if ((flags==MAP_DENYWRITE) && (curproc->p_files[fd]->f_mode==FMODE_WRITE))
        		return -ETXTBSY;*/
        	tlb_flush((uintptr_t)addr);
        	/*what should be lopage and npage*/
        	uint32_t lopage = ADDR_TO_PN(addr);
        	uint32_t npages = len / PAGE_SIZE + 1;
        	int i = vmmap_map(curproc->p_vmmap, curproc->p_files[fd]->f_vnode, lopage, npages, prot, flags, off, VMMAP_DIR_HILO, (vmarea_t**)ret);
        	return 0;
}


/*
 * This function implements the munmap(2) syscall.
 *
 * As with do_mmap() it should perform the required error checking,
 * before calling upon vmmap_remove() to do most of the work.
 * Remember to clear the TLB.
 */
int
do_munmap(void *addr, size_t len)
{
       /* NOT_YET_IMPLEMENTED("VM: do_munmap");
        return -1;*/
        if (len==0)
        		return -EINVAL;

        	if(PAGE_ALIGNED(addr)==0)
        		return -EINVAL;
        	tlb_flush((uintptr_t)addr);
        	uint32_t lopage = ADDR_TO_PN(addr);
        	uint32_t npages = len / PAGE_SIZE + 1;
        	vmmap_remove(curproc->p_vmmap,lopage,npages);
        	return 0;
}

