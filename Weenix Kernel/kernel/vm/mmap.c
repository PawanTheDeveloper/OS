

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
   /* NOT_YET_IMPLEMENTED("VM: do_mmap"); */

	int inv = 0;
	int return_value;
	uint32_t pages = 0;
	uint32_t newaddr = (uint32_t)addr;
	uintptr_t addr_ptr = 0;
	
	/*Addr Checking*/
	if ( addr != NULL )
	{
		if( (newaddr < USER_MEM_LOW) || (newaddr > USER_MEM_HIGH) )
		{
			inv=1;
			dbg(DBG_PRINT, "\n GRADING3D INVALID : address is out of bound \n");
		}
	}
	if ( addr == NULL )
	{
		if( (flags == 0) || (flags & MAP_FIXED) || (flags == MAP_TYPE) )
		{
			inv=1;
		}
	}

	/* Length Checking*/

	if(len == 0)
	{
		inv=1;
	}
	if ((sizeof(len) == NULL) || (len == (size_t) - 1) )
	{
		inv=1;
	}

	if ( (newaddr + len) > USER_MEM_HIGH )
	{
		inv=1;
	}
	
	if (!PAGE_ALIGNED(off))
	{
		inv=1;
	}

	if(inv)
	{
		return -EINVAL;
	}

	vnode_t *vnode = NULL;
	file_t *f2;

	if ( ! ( flags & MAP_ANON ) )
	{
		if ( (fd < 0) || (fd >= NFILES) || (curproc->p_files[fd] == NULL) )
		{
			return -EBADF;
		}

		f2 = fget(fd);
		if ( f2 != NULL )
		{
			vnode = f2->f_vnode;
			fput(f2);
		}

	}

	if ( flags == MAP_SHARED )
	{
		if ( prot & PROT_WRITE )
		{
			if ( ( ! ( curproc->p_files[fd]->f_mode & FMODE_READ ) ) || ( ! ( curproc->p_files[fd]->f_mode & FMODE_WRITE ) ) )
			{
				return -EACCES;
			}
		}
	}

	while ( len > PAGE_SIZE )
	{

            len -= PAGE_SIZE;
            pages++;
    }
	
    if( len > 0 )
	{
            pages++;
    }

	vmarea_t *vma;
	pagedir_t *pd = pt_get();

	int result = vmmap_map(curproc->p_vmmap, vnode, ADDR_TO_PN(addr), pages, prot, flags, off, VMMAP_DIR_HILO, &vma);

	if (((uintptr_t)PN_TO_ADDR(vma->vma_end) > USER_MEM_HIGH) || ((uintptr_t)PN_TO_ADDR(vma->vma_start) < USER_MEM_LOW))
	{
            return -1;
    }

	if ( result >= 0 && ret != NULL )
	{
		if (addr == NULL)
		{
		        addr_ptr = (uintptr_t)PN_TO_ADDR(vma->vma_start);
			
			tlb_flush_range(addr_ptr, pages);
		    pt_unmap_range(pd, addr_ptr, (uintptr_t)PN_TO_ADDR(vma->vma_start + pages));
		}
		else
		{
		        addr_ptr = (uintptr_t)addr;
		     
			tlb_flush_range(addr_ptr, pages);
		    pt_unmap_range(pd, addr_ptr, addr_ptr + (uintptr_t)(pages * PAGE_SIZE));
		}
		if( ret )
			{
				*ret = (void*)addr_ptr;
			}
	}

	KASSERT(NULL != curproc->p_pagedir);
	
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
        /* NOT_YET_IMPLEMENTED("VM: do_munmap"); */

	int inv = 0;
	uint32_t pages = 0;
	uint32_t newaddr = (uint32_t)addr;

	if ( len == 0 || len == (size_t) - 1 )
	{
		inv = 1;
		dbg(DBG_PRINT, "GRADING3D INVALID : invalid length \n");
	}
	
	if ( addr == NULL )
	{
		if( newaddr < USER_MEM_LOW || newaddr > USER_MEM_HIGH )
		{
			inv = 1;
			dbg(DBG_PRINT, "GRADING3D INVALID : ADDRESS bound \n");
		}
		else
		{
			inv = 1;
			dbg(DBG_PRINT, "GRADING3D INVALID : addr is NULL \n");
		}
	}

	if ( (newaddr + len) > USER_MEM_HIGH )
	{
		inv = 1;
		dbg(DBG_PRINT, "GRADING3D INVALID : OUT of RANGE \n");
	}

	if ( ! PAGE_ALIGNED(addr) )
	{
		inv = 1;
		dbg(DBG_PRINT, "GRADING3D INVALID : addr is not aligned \n");
	}

	if ( inv == 1 )
	{
		dbg(DBG_PRINT, "GRADING3D INVALID : returning INVALID code \n");
		return -EINVAL;
	}

	while ( len > PAGE_SIZE )
	{
                len = len - PAGE_SIZE;
                pages++;
        }
        if( len > 0 )
	{
		dbg(DBG_PRINT, "GRADING3D length is still > 0 \n");
                pages++;
        }

        dbg(DBG_PRINT,"About to call VMMAP_REMOVE.\n");
        int return_value = vmmap_remove( curproc->p_vmmap, ADDR_TO_PN(addr), (uint32_t)pages );
        tlb_flush_range( (uintptr_t)addr, pages );

        if (((uintptr_t)addr >= USER_MEM_LOW) && ((uintptr_t)PN_TO_ADDR(ADDR_TO_PN(addr) + pages) <= USER_MEM_HIGH))
	{
		dbg(DBG_PRINT, "GRADING3D add >= Low and (addr + pages <) High : within mem limits \n");
                pt_unmap_range(pt_get(), (uintptr_t)addr, (uintptr_t)PN_TO_ADDR(ADDR_TO_PN(addr) + pages));
        }

        return 0;
}

