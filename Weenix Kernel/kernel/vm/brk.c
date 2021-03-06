

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
        /* NOT_YET_IMPLEMENTED("VM: do_brk"); */

	dbg( DBG_PRINT, "\n GRADING3D ENTERING do_brk() \n");

	vmarea_t *vmarea = NULL;
		
	if(addr == NULL)
	{
		dbg( DBG_PRINT, "\n GRADING3D INSIDE do_brk() : addr is null \n");
		*ret=curproc->p_brk;
		return 0;
	}

	if(addr < curproc->p_start_brk || (uint32_t)addr > USER_MEM_HIGH)
	{
		dbg( DBG_PRINT, "\n GRADING3D INSIDE do_brk() : addr is < start_brk or > MEM_HIGH. return ENOMEM.  \n");
		return -ENOMEM;
	}

	uint32_t brk_first_page = ADDR_TO_PN(PAGE_ALIGN_UP(curproc->p_brk));
	uint32_t old_brk = (uint32_t)curproc->p_brk;
	uint32_t brk_end_page = ADDR_TO_PN((uintptr_t)addr-1);
	vmarea = vmmap_lookup(curproc->p_vmmap,ADDR_TO_PN(PAGE_ALIGN_DOWN((uintptr_t)curproc->p_start_brk)));
	if(vmarea->vma_end <= brk_end_page)
	{
		dbg( DBG_PRINT, "\n GRADING3D INSIDE do_brk() : vmarea_end <= brk_end_page \n");

		if(!vmmap_is_range_empty(curproc->p_vmmap, vmarea->vma_end, brk_end_page - vmarea->vma_end + 1))
		{
			dbg( DBG_PRINT, "\n GRADING3D INSIDE do_brk() : vmmap range is not empty. returning ENOMEM \n");
			return -ENOMEM;
		}
	}
	curproc->p_brk = addr;
	*ret = addr;
	vmarea->vma_end = brk_end_page + 1;
	dbg( DBG_PRINT, "\n GRADING3D EXITING do_brk() \n");
	return 0;
}
