

#include "types.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/string.h"

#include "proc/proc.h"
#include "proc/kthread.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/pframe.h"
#include "mm/mmobj.h"
#include "mm/pagetable.h"
#include "mm/tlb.h"

#include "fs/file.h"
#include "fs/vnode.h"

#include "vm/shadow.h"
#include "vm/vmmap.h"

#include "api/exec.h"

#include "main/interrupt.h"

/* Pushes the appropriate things onto the kernel stack of a newly forked thread
 * so that it can begin execution in userland_entry.
 * regs: registers the new thread should have on execution
 * kstack: location of the new thread's kernel stack
 * Returns the new stack pointer on success. */
static uint32_t
fork_setup_stack(const regs_t *regs, void *kstack)
{
        /* Pointer argument and dummy return address, and userland dummy return
         * address */
        uint32_t esp = ((uint32_t) kstack) + DEFAULT_STACK_SIZE - (sizeof(regs_t) + 12);
        *(void **)(esp + 4) = (void *)(esp + 8); /* Set the argument to point to location of struct on stack */
        memcpy((void *)(esp + 8), regs, sizeof(regs_t)); /* Copy over struct */
        return esp;
}


/*
 * The implementation of fork(2). Once this works,
 * you're practically home free. This is what the
 * entirety of Weenix has been leading up to.
 * Go forth and conquer.
 */
int
do_fork(struct regs *regs)
{

	/*

	1. Allocate a proc_t out of the procs structure using proc_create().
	2. Copy the vmmap_t from the parent process into the child using vmmap_clone(). Remember to increase the reference counts on the underlying mmobj_ts.
	3. For each private mapping, point the vmarea_t at the new shadow object, which in turn should point to the original mmobj_t for the vmarea_t. This is how you know that the pages corresponding to this mapping are copy-on-write. Be careful with reference counts. Also note that for shared mappings, there is no need to copy the mmobj_t.
	4. Unmap the user land page table entries and flush the TLB (using pt_unmap_range() and tlb_flush_all()). This is necessary because the parent process might still have some entries marked as "writable", but since we are implementing copy-on-write we would like access to these pages to cause a trap.
	5. Set up the new process thread context (kt_ctx). You will need to set the following:
		a. c_pdptr - the page table pointer
		b. c_eip - function pointer for the userland_entry function
		c. c_esp - the value returned by fork_setup_stack()
		d. c_kstack - the top of the new thread's kernel stack
		e. c_kstacksz - size of the new thread's kernel stack
		f. Remember to set the return value in the child process!
	6. Copy the file descriptor table of the parent into the child. Remember to use fref() here.
	7. Set the child's working directory to point to the parent's working directory (once again, remember reference counts).
	8. Use kthread_clone() to copy the thread from the parent process into the child process.
	9. Set any other fields in the new process which need to be set.
	10. Make the new thread runnable.

	*/

        /* NOT_YET_IMPLEMENTED("VM: do_fork"); */

	KASSERT(regs != NULL);
	dbg ( DBG_PRINT, "\n GRADING3A 7.a KASSERT(regs != NULL) is successful. ");

	KASSERT(curproc != NULL);
	dbg ( DBG_PRINT, "\n GRADING3A 7.a KASSERT(curproc != NULL) is successful. ");

	KASSERT(curproc->p_state == PROC_RUNNING);
	dbg ( DBG_PRINT, "\n GRADING3A 7.a KASSERT(curproc->p_state == PROC_RUNNING) is successful. ");

	/* Step - 1 */
	proc_t *childproc = proc_create("Child_Process");
	vmmap_destroy(childproc->p_vmmap);

	/* Step - 2 */
	vmmap_t *cvmmap = vmmap_clone ( curproc->p_vmmap );
	

	/* Step - 3 */
	vmarea_t *pvma = NULL;
	vmarea_t *cvma = NULL;
	mmobj_t *ob;

	list_iterate_begin( &(curproc->p_vmmap->vmm_list), pvma, vmarea_t, vma_plink )
	{
		cvma = vmmap_lookup(cvmmap, pvma->vma_start);

		if ( pvma->vma_flags & MAP_PRIVATE )
		{
			dbg ( DBG_PRINT, "\n GRADING3D do_fork : in case of private mapping ");

			ob = mmobj_bottom_obj(pvma->vma_obj);

			mmobj_t *csobj = shadow_create();
                        csobj->mmo_shadowed = pvma->vma_obj;
			csobj->mmo_un.mmo_bottom_obj = mmobj_bottom_obj(pvma->vma_obj);
			cvma->vma_obj = csobj;

			list_insert_head(&(ob->mmo_un.mmo_vmas), &(cvma->vma_olink));

			mmobj_t *psobj = shadow_create();
			psobj->mmo_shadowed = pvma->vma_obj;
			psobj->mmo_un.mmo_bottom_obj = mmobj_bottom_obj(pvma->vma_obj);
			pvma->vma_obj = psobj;
		}
	} list_iterate_end();

	childproc->p_vmmap = cvmmap;
	cvmmap->vmm_proc = childproc;


	/* Step - 4 */
	pt_unmap_range(curproc->p_pagedir, USER_MEM_LOW, USER_MEM_HIGH);
	tlb_flush_all();

	KASSERT(childproc->p_state == PROC_RUNNING);
	dbg ( DBG_PRINT, "\n GRADING3A 7.a KASSERT(newproc->p_state == PROC_RUNNING) is successful. ");

	KASSERT(childproc->p_pagedir != NULL);
	dbg ( DBG_PRINT, "\n GRADING3A 7.a KASSERT(newproc->p_pagedir != NULL) is successful. ");


	/* Step - 5 & 8 */
	kthread_t *childthr = kthread_clone(curthr);


	KASSERT(childthr->kt_kstack != NULL);
	dbg ( DBG_PRINT, "\n GRADING3A 7.a KASSERT(childthr->kt_kstack != NULL) is successful. ");

	regs->r_eax = 0;

	/* 5.a */ childthr->kt_ctx.c_pdptr = childproc->p_pagedir;
	/* 5.b */ childthr->kt_ctx.c_eip = (uintptr_t)userland_entry;
	/* 5.c */ childthr->kt_ctx.c_esp = fork_setup_stack(regs, childthr->kt_kstack);
	/* 5.d */ childthr->kt_ctx.c_kstack = (uintptr_t)childthr->kt_kstack;
	/* 5.e */ childthr->kt_ctx.c_kstacksz = DEFAULT_STACK_SIZE;


	list_insert_tail( &(childproc->p_threads), &(childthr->kt_plink) );

	/* Step - 6 */
	childthr->kt_proc = childproc;

	int i = 0;
	while ( i < NFILES )
	{
		childproc->p_files[i] = curproc->p_files[i];
		if ( childproc->p_files[i] != NULL )
		{
			dbg ( DBG_PRINT, "/n GRADING3D do_fork : when pfiles are not null. Then fref ");
			fref(childproc->p_files[i]);
		}
		i++;
	}


	/* Step - 7 */
	childproc->p_cwd = curproc->p_cwd;
	

	/* Step - 9 */
	childproc->p_brk = curproc->p_brk;
	childproc->p_start_brk = curproc->p_start_brk;


	/* Step - 10 */
	sched_make_runnable(childthr);

        return childproc->p_pid;
}
