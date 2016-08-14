

#include "kernel.h"
#include "errno.h"
#include "globals.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "proc/proc.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/fcntl.h"
#include "fs/vfs_syscall.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/mmobj.h"

static slab_allocator_t *vmmap_allocator;
static slab_allocator_t *vmarea_allocator;

void
vmmap_init(void)
{
        vmmap_allocator = slab_allocator_create("vmmap", sizeof(vmmap_t));
        KASSERT(NULL != vmmap_allocator && "failed to create vmmap allocator!");
        vmarea_allocator = slab_allocator_create("vmarea", sizeof(vmarea_t));
        KASSERT(NULL != vmarea_allocator && "failed to create vmarea allocator!");
}

vmarea_t *
vmarea_alloc(void)
{
        vmarea_t *newvma = (vmarea_t *) slab_obj_alloc(vmarea_allocator);
        if (newvma) {
                newvma->vma_vmmap = NULL;
        }
        return newvma;
}

void
vmarea_free(vmarea_t *vma)
{
        KASSERT(NULL != vma);
        slab_obj_free(vmarea_allocator, vma);
}

/* a debugging routine: dumps the mappings of the given address space. */
size_t
vmmap_mapping_info(const void *vmmap, char *buf, size_t osize)
{
        KASSERT(0 < osize);
        KASSERT(NULL != buf);
        KASSERT(NULL != vmmap);

        vmmap_t *map = (vmmap_t *)vmmap;
        vmarea_t *vma;
        ssize_t size = (ssize_t)osize;

        int len = snprintf(buf, size, "%21s %5s %7s %8s %10s %12s\n",
                           "VADDR RANGE", "PROT", "FLAGS", "MMOBJ", "OFFSET",
                           "VFN RANGE");

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                size -= len;
                buf += len;
                if (0 >= size) {
                        goto end;
                }

                len = snprintf(buf, size,
                               "%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
                               vma->vma_start << PAGE_SHIFT,
                               vma->vma_end << PAGE_SHIFT,
                               (vma->vma_prot & PROT_READ ? 'r' : '-'),
                               (vma->vma_prot & PROT_WRITE ? 'w' : '-'),
                               (vma->vma_prot & PROT_EXEC ? 'x' : '-'),
                               (vma->vma_flags & MAP_SHARED ? " SHARED" : "PRIVATE"),
                               vma->vma_obj, vma->vma_off, vma->vma_start, vma->vma_end);
        } list_iterate_end();

end:
        if (size <= 0) {
                size = osize;
                buf[osize - 1] = '\0';
        }
        /*
        KASSERT(0 <= size);
        if (0 == size) {
                size++;
                buf--;
                buf[0] = '\0';
        }
        */
        return osize - size;
}

/* Create a new vmmap, which has no vmareas and does
 * not refer to a process. */
vmmap_t *
vmmap_create(void)
{
        /* NOT_YET_IMPLEMENTED("VM: vmmap_create"); */
	
	vmmap_t *newmap = (vmmap_t*)slab_obj_alloc(vmmap_allocator);
	if(newmap == NULL)
	{
		dbg(DBG_PRINT,"\n GRADING3D vmmap_create : Not able to create new vmmap, thus returning NULL\n");
		return NULL;
	}
	else
	{
		dbg(DBG_PRINT,"\n GRADING3D vmmap_create : new vmmap created successfully. Now, initializing list\n");
		newmap->vmm_proc = NULL;
		list_init( &(newmap->vmm_list) );
	}

	dbg(DBG_PRINT,"\n GRADING3D vmmap_create : returning new vmmap\n");
        return newmap;
}

/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void
vmmap_destroy(vmmap_t *map)
{
	/* NOT_YET_IMPLEMENTED("VM: vmmap_destroy"); */

	KASSERT(NULL != map);
	dbg(DBG_PRINT,"\n GRADING3A 3.a KASSERT(NULL != map); SUCCESSFUL\n");

	vmarea_t *vmarea;
	if( list_empty( &(map->vmm_list) ) )
	{
		dbg(DBG_PRINT,"\n GRADING3D vmmap_destory : The list is empty, thus returning.\n");
		return;
	}
	/*
	Iterate the Vmareas of the map.
	Here maps is part of PCB, every map contains mulitple vmarea. Vmarea itself 
	contains a few details like . Vmarea is part of what map. 2) A linked list of VM area, this can happen because while removing the content from in between
	Vmarea's can get split. Thus we need to have a link list, so after seperating they can be attached through link list. 
	What you need to do here is parse all the vmarea inside the maps. and then remove or NULL the components 1 by 1.
	*/
	list_iterate_begin( &(map->vmm_list), vmarea, vmarea_t, vma_plink )
	{
		/* Remove the plink list*/
		list_remove( &(vmarea->vma_plink) );

		if ( list_link_is_linked (&vmarea->vma_olink) )
		{
			dbg(DBG_PRINT,"\n GRADING3D vmmap_destory : list link is linked.\n");
			list_remove( &(vmarea->vma_olink) );
		}

		vmarea->vma_obj->mmo_ops->put( vmarea->vma_obj );
		vmarea_free(vmarea);

	}list_iterate_end();

	slab_obj_free(vmmap_allocator, map);
	dbg(DBG_PRINT,"\n GRADING3D vmmap_destory : exiting function after freeing slab obj.\n");
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
void
vmmap_insert(vmmap_t *map, vmarea_t *newvma)
{
	/* NOT_YET_IMPLEMENTED("VM: vmmap_insert"); */

	/* Last part of Comment*/

	KASSERT(NULL != map && NULL != newvma);
	dbg(DBG_PRINT,"\n GRADING3A 3.b KASSERT(NULL != map && NULL != newvma); SUCCESSFUL \n");

	KASSERT(NULL == newvma->vma_vmmap);
	dbg(DBG_PRINT,"\n GRADING3A 3.b KASSERT(NULL == newvma->vma_vmmap); SUCCESSFUL \n");

	KASSERT(newvma->vma_start < newvma->vma_end);
	dbg(DBG_PRINT,"\n GRADING3A 3.b KASSERT(newvma->vma_start < newvma->vma_end); SUCCESSFUL \n");

	KASSERT(ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start && ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end);
	dbg(DBG_PRINT,"\n GRADING3A 3.b KASSERT(ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start && ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end); SUCCESSFUL \n");

	newvma->vma_vmmap = map;
	
	if(map == NULL || newvma == NULL)
	{
		dbg(DBG_PRINT,"\n GRADING3D vmmap_insert : Both the parameters are NULL, thus returing without any calculations from VMmap Insert");
		return;
	}
	/*
		We need to parse the linked list of plink, find the required place, insert the vmarea just like
		inserting a node in the link list.
		Remember:
		1) If the link is empty , that means there is no other vmarea, this is the first one, then you can 
		include the current vmarea inside the vmarea_plink.(Ironically, this linked list will exist in the current vmarea itself)
		2) If there are already mulitple vmareas , we need to parse it,and find the given vmarea and then insert our new vmarea before that.
		3)  
	*/
	if( !(list_empty( &(map->vmm_list) )) )
	{
		dbg(DBG_PRINT,"\n GRADING3D vmmap_insert : list is not empty");
		vmarea_t *freshvma;
		list_iterate_begin(&map->vmm_list, freshvma, vmarea_t, vma_plink)
		{
			if( (freshvma->vma_start) > (newvma->vma_start) )
			{
				dbg(DBG_PRINT,"\n GRADING3D vmmap_insert : freshvma_start > newvma_start. Returning");
				list_insert_before( &freshvma->vma_plink, &newvma->vma_plink );
				return;
			}

		}list_iterate_end();
		list_insert_tail( &map->vmm_list, &newvma->vma_plink );
	}
	else
	{
		dbg(DBG_PRINT,"\n GRADING3D vmmap_insert : inserting newvma at the tail of the map list");
		list_insert_tail( &map->vmm_list, &newvma->vma_plink );
	}
}

/* Find a contiguous range of free virtual pages of length npages in
 * the given address space. Returns starting vfn for the range,
 * without altering the map. Returns -1 if no such range exists.
 *
 * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
 * should find a gap as high in the address space as possible; if dir
 * is VMMAP_DIR_LOHI, the gap should be as low as possible. */
int
vmmap_find_range(vmmap_t *map, uint32_t npages, int dir)
{
        /* NOT_YET_IMPLEMENTED("VM: vmmap_find_range"); */

	vmarea_t *vmarea;
	int is_range_empty = 0;

	if(map == NULL)
	{
		dbg(DBG_PRINT,"\n GRADING3D vmmap_find_range : map is null. Returning ");
		return -1;
	}

	if(dir == VMMAP_DIR_HILO)
	{
		dbg(DBG_PRINT,"\n GRADING3D vmmap_find_range : dir = VMMAP_DIR_HILO ");
		list_iterate_reverse(&(map->vmm_list), vmarea, vmarea_t, vma_plink)
		{
			if( vmarea->vma_plink.l_next == &(map->vmm_list) )
			{
				dbg(DBG_PRINT,"\n GRADING3D vmmap_find_range : vmarea->vma_plink.l_next == &(map->vmm_list) ");
				if ( ADDR_TO_PN(USER_MEM_HIGH) >= (vmarea->vma_end + npages) )
				{
					dbg(DBG_PRINT,"GRADING3D vmmap_find_range : end + npages <= MEM_HIGH. returning value ");
					return (ADDR_TO_PN(USER_MEM_HIGH) - npages);
				}
			}

			if ( ADDR_TO_PN(USER_MEM_LOW) <= (vmarea->vma_start - npages) )
			{
				dbg(DBG_PRINT,"\n GRADING3D vmmap_find_range : start - npages <= MEM_LOW. ");
				is_range_empty = vmmap_is_range_empty(map, vmarea->vma_start - npages, npages);
				if(is_range_empty != 0)
				{
					dbg(DBG_PRINT,"\n GRADING3D vmmap_find_range : range is not empty ");
					return (vmarea->vma_start - npages);
				}
				else
				{
					dbg(DBG_PRINT,"\n GRADING3D vmmap_find_range : range is empty ");
					continue;
				}
			}

		}list_iterate_end();
	}

	dbg(DBG_PRINT,"\n GRADING3D vmmap_find_range : returnung from function ");
	return -1;
}

/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn)
{
	/* NOT_YET_IMPLEMENTED("VM: vmmap_lookup"); */

	/* Just iterate as above , if found return.*/

	KASSERT(NULL != map);
	dbg(DBG_PRINT,"\n GRADING3A 3.c KASSERT(NULL != map); SUCCESSFUL \n");

	vmarea_t *vmarea = NULL;

	list_iterate_begin(&(map->vmm_list), vmarea, vmarea_t, vma_plink)
	{
		if((vmarea->vma_start <= vfn ) && (vmarea->vma_end > vfn))
		{
			dbg(DBG_PRINT,"\n GRADING3D vmmap_lookup : returnung vmarea ");
			return vmarea;
		}

	}list_iterate_end();

	dbg(DBG_PRINT,"\n GRADING3D vmmap_lookup : returnung null ");
	return NULL;
}

/* Allocates a new vmmap containing a new vmarea for each area in the
 * given map. The areas should have no mmobjs set yet. Returns pointer
 * to the new vmmap on success, NULL on failure. This function is
 * called when implementing fork(2). */
vmmap_t *
vmmap_clone(vmmap_t *map)
{
        /* NOT_YET_IMPLEMENTED("VM: vmmap_clone"); */

	vmarea_t *vmarea = NULL;
	vmmap_t *vmmap = NULL;
	vmmap = vmmap_create();
	if(vmmap == NULL)
	{
		dbg(DBG_PRINT, "\n GRADING3D vmmap_clone : Not able to create a new vmmap.\n");
		return NULL;
	}

	list_iterate_begin(&map->vmm_list, vmarea, vmarea_t, vma_plink)
	{
	    	vmarea_t *newvmarea = (vmarea_t *) slab_obj_alloc(vmarea_allocator);
		if(newvmarea == NULL)
		{
			dbg(DBG_PRINT, "\n GRADING3D : vmmap_clone : Not able to create new vmarea. EXITING \n");
			return NULL;
		}
		newvmarea->vma_start = vmarea->vma_start;
		newvmarea->vma_end = vmarea->vma_end;
		newvmarea->vma_off = vmarea->vma_off;
		newvmarea->vma_prot = vmarea->vma_prot;
		newvmarea->vma_flags = vmarea->vma_flags;
		newvmarea->vma_vmmap = NULL;
		newvmarea->vma_obj = vmarea->vma_obj;
		newvmarea->vma_obj->mmo_ops->ref( newvmarea->vma_obj );
		list_link_init( &newvmarea->vma_plink );
		list_link_init( &newvmarea->vma_olink );
		vmmap_insert( vmmap, newvmarea );

	}list_iterate_end();

	dbg(DBG_PRINT, "\n GRADING3D vmmap_clone : returning new vmmap.\n");
	return vmmap;
}

/* Insert a mapping into the map starting at lopage for npages pages.
 * If lopage is zero, we will find a range of virtual addresses in the
 * process that is big enough, by using vmmap_find_range with the same
 * dir argument.  If lopage is non-zero and the specified region
 * contains another mapping that mapping should be unmapped.
 *
 * If file is NULL an anon mmobj will be used to create a mapping
 * of 0's.  If file is non-null that vnode's file will be mapped in
 * for the given range.  Use the vnode's mmap operation to get the
 * mmobj for the file; do not assume it is file->vn_obj. Make sure all
 * of the area's fields except for vma_obj have been set before
 * calling mmap.
 *
 * If MAP_PRIVATE is specified set up a shadow object for the mmobj.
 *
 * All of the input to this function should be valid (KASSERT!).
 * See mmap(2) for for description of legal input.
 * Note that off should be page aligned.
 *
 * Be very careful about the order operations are performed in here. Some
 * operation are impossible to undo and should be saved until there
 * is no chance of failure.
 *
 * If 'new' is non-NULL a pointer to the new vmarea_t should be stored in it.
 */
int
vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,
          int prot, int flags, off_t off, int dir, vmarea_t **new)
{
        /* NOT_YET_IMPLEMENTED("VM: vmmap_map"); */

	KASSERT(NULL != map);
	dbg(DBG_PRINT,"\n GRADING3A 3.d KASSERT(NULL != map); SUCCESSFUL \n");

	KASSERT(0 < npages);
	dbg(DBG_PRINT,"\n GRADING3A 3.d KASSERT(0 < npages); SUCCESSFUL \n");

	KASSERT((MAP_SHARED & flags) || (MAP_PRIVATE & flags));
	dbg(DBG_PRINT,"\n GRADING3A 3.d KASSERT((MAP_SHARED & flags) || (MAP_PRIVATE & flags)); SUCCESSFUL \n");

	KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_LOW) <= lopage));
	dbg(DBG_PRINT,"\n GRADING3A 3.d KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_LOW) <= lopage)); SUCCESSFUL \n");

	KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_HIGH) >= (lopage + npages)));
	dbg(DBG_PRINT,"\n GRADING3A 3.d KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_HIGH) >= (lopage + npages))); SUCCESSFUL \n");

	KASSERT(PAGE_ALIGNED(off));
	dbg(DBG_PRINT,"\n GRADING3A 3.d KASSERT(PAGE_ALIGNED(off)); SUCCESSFUL \n");

	vmarea_t *vmarea;
	if(lopage == 0)
	{
		dbg(DBG_PRINT, "\n GRADING3D vmmap_map : lopage is 0.\n");
		int start_addr = 0;
		start_addr = vmmap_find_range(map, npages, dir);
		if(start_addr < 0)
		{
		    dbg(DBG_PRINT, "\n GRADING3D vmmap_map : start_addr < 0. ENOMEM . EXITING\n");
		    return -ENOMEM;
		}

		vmarea = vmarea_alloc();
		if(vmarea == NULL)
		{
			dbg(DBG_PRINT, "\n GRADING3D vmmap_map : couldn't allocate newvmarea. ENOMEM . EXITING\n");
			return -ENOMEM;
		}

		vmarea->vma_start = start_addr;
		vmarea->vma_end = start_addr+npages;
		vmarea->vma_prot = prot;
		vmarea->vma_flags = flags;
		vmarea->vma_off = ADDR_TO_PN(off);
		list_link_init( &(vmarea->vma_plink) );
		list_link_init( &(vmarea->vma_olink) );
		
		/*Condition for Files NULL */
		if(file)
		{
			dbg(DBG_PRINT, "\n GRADING3D vmmap_map : file obj is not null\n");
		    mmobj_t *file_obj;
		    file->vn_ops->mmap(file, vmarea, &file_obj);
		    vmarea->vma_obj = file_obj;
		}
		else
		{
			dbg(DBG_PRINT, "\n GRADING3D vmmap_map : file obj is null\n");
		    mmobj_t *anon_obj = anon_create();
		    vmarea->vma_obj = anon_obj;
		}

		if(flags & MAP_PRIVATE)
		{
			dbg(DBG_PRINT, "\n GRADING3D vmmap_map : flag is private \n");
		    mmobj_t *shadow_obj = shadow_create();
		    shadow_obj->mmo_shadowed = vmarea->vma_obj;
		    shadow_obj->mmo_un.mmo_bottom_obj = mmobj_bottom_obj( vmarea->vma_obj );
		    list_insert_head( &vmarea->vma_obj->mmo_un.mmo_vmas, &vmarea->vma_olink );
		    vmarea->vma_obj = shadow_obj;
		}
	}
	else
	{
		dbg(DBG_PRINT, "\n GRADING3D vmmap_map : lopage is not  0 \n");
		if((vmmap_is_range_empty(map, lopage, npages)) == 0)
		{
			dbg(DBG_PRINT, "\n GRADING3D vmmap_map : calling vmmap_remove \n");
			vmmap_remove(map, lopage, npages);
		}

		vmarea = vmarea_alloc();
		if(vmarea == NULL)
		{
		    dbg(DBG_PRINT,"\n GRADING3D vmmap_map : Could not create new vmarea. ENOMEM\n");
		    return -ENOMEM;   
		}

		vmarea->vma_start = lopage;
		vmarea->vma_end = lopage+npages;
		vmarea->vma_prot = prot;
		vmarea->vma_flags = flags;
		vmarea->vma_off = ADDR_TO_PN(off);
		list_link_init( &(vmarea->vma_plink) );
		list_link_init( &(vmarea->vma_olink) );

		/*Condition for Files NULL */
		if(file)
		{
			dbg(DBG_PRINT, "\n GRADING3D vmmap_map : file obj is not null\n");
		    mmobj_t *file_obj;
		    file->vn_ops->mmap(file, vmarea, &file_obj);
		    vmarea->vma_obj=file_obj;
		}
		else
		{
			dbg(DBG_PRINT, "\n GRADING3D vmmap_map : file obj is null\n");
		    mmobj_t *anon_obj = anon_create();
		    vmarea->vma_obj = anon_obj;
		}

		if(flags & MAP_PRIVATE)
		{
			dbg(DBG_PRINT, "\n GRADING3D vmmap_map : flag is private \n");
		    mmobj_t *shadow_obj = shadow_create();
		    shadow_obj->mmo_shadowed = vmarea->vma_obj;
		    shadow_obj->mmo_un.mmo_bottom_obj = mmobj_bottom_obj( vmarea->vma_obj );
		    list_insert_head( &vmarea->vma_obj->mmo_un.mmo_vmas, &vmarea->vma_olink );
		    vmarea->vma_obj = shadow_obj;
		}
	}
	vmmap_insert(map,vmarea);

	if(new != NULL)
	{
		dbg(DBG_PRINT, "\n GRADING3D vmmap_map : new* is not null \n");
        	*new = vmarea;
	}

	dbg(DBG_PRINT, "\n GRADING3D vmmap_map : returning from function \n");
	return 0;
}

/*
 * We have no guarantee that the region of the address space being
 * unmapped will play nicely with our list of vmareas.
 *
 * You must iterate over each vmarea that is partially or wholly covered
 * by the address range [addr ... addr+len). The vm-area will fall into one
 * of four cases, as illustrated below:
 *
 * key:
 *          [             ]   Existing VM Area
 *        *******             Region to be unmapped
 *
 * Case 1:  [   ******    ]
 * The region to be unmapped lies completely inside the vmarea. We need to
 * split the old vmarea into two vmareas. be sure to increment the
 * reference count to the file associated with the vmarea.
 *
 * Case 2:  [      *******]**
 * The region overlaps the end of the vmarea. Just shorten the length of
 * the mapping.
 *
 * Case 3: *[*****        ]
 * The region overlaps the beginning of the vmarea. Move the beginning of
 * the mapping (remember to update vma_off), and shorten its length.
 *
 * Case 4: *[*************]**
 * The region completely contains the vmarea. Remove the vmarea from the
 * list.
 */
int
vmmap_remove(vmmap_t *map, uint32_t lopage, uint32_t npages)
{
/* NOT_YET_IMPLEMENTED("VM: vmmap_remove"); */

	vmarea_t *vmarea;
	vmarea_t *secondvmarea;
	uint32_t end;

	if( vmmap_is_range_empty(map, lopage, npages) )
	{
		dbg(DBG_PRINT, "\n GRADING3D vmmap_remove : vmmap range is empty \n");
		return 0;
        }
    
	if( !(list_empty(&(map->vmm_list))))
	{
		dbg(DBG_PRINT, "\n GRADING3D vmmap_remove : list is not empty \n");
		list_iterate_begin(&map->vmm_list, vmarea, vmarea_t, vma_plink)
		{
			end = lopage + npages;

			if ( vmarea->vma_start >= (lopage + npages) || vmarea->vma_end <= lopage )
			{
				dbg(DBG_PRINT,"\n GRADING3D vmmap_remove : Case 5.\n");
				continue;
			}
			else if(((vmarea->vma_start) < lopage) && ((vmarea->vma_end) > end))
			{
				dbg(DBG_PRINT,"\n GRADING3D vmmap_remove : Case 1.\n");
				secondvmarea = vmarea_alloc();
				if(secondvmarea == NULL)
				{
				    dbg(DBG_PRINT,"\n GRADING3D vmmap_remove : Case 1. couldn't allocate vmarea \n");
				    return NULL;
				}
				secondvmarea->vma_start = vmarea->vma_start;
				secondvmarea->vma_end = lopage;
				secondvmarea->vma_obj = vmarea->vma_obj;
				secondvmarea->vma_off = vmarea->vma_off;
				secondvmarea->vma_prot = vmarea->vma_prot;
				secondvmarea->vma_flags = vmarea->vma_flags;
				secondvmarea->vma_vmmap = vmarea->vma_vmmap;
				list_link_init( &secondvmarea->vma_plink );
				list_link_init( &secondvmarea->vma_olink );
				secondvmarea->vma_obj->mmo_ops->ref(secondvmarea->vma_obj);
				/*vmmap_insert(map, secondvmarea);*/
				list_insert_before(&(vmarea->vma_plink), &(secondvmarea->vma_plink));

				mmobj_t *newmmobj = mmobj_bottom_obj(vmarea->vma_obj);
				if(newmmobj != vmarea->vma_obj)
				{
				    list_insert_head( &(newmmobj->mmo_un.mmo_vmas), &(secondvmarea->vma_olink) );
				}
				vmarea->vma_off = lopage-vmarea->vma_start + npages + vmarea->vma_off;
				vmarea->vma_start = end;
				return 0;
			}
			else if((vmarea->vma_start < lopage) && ((vmarea->vma_end) <= end))
			{
				dbg(DBG_PRINT,"\n GRADING3D vmmap_remove : Case 2.\n");
				vmarea->vma_end = lopage;
				continue;
			}
			else if((vmarea->vma_start >= lopage) && (vmarea->vma_end > end))
			{
				dbg(DBG_PRINT,"\n GRADING3D vmmap_remove : Case 3.\n");
				vmarea->vma_off = vmarea->vma_off + end - vmarea->vma_start;
				vmarea->vma_start = end;	
				continue;
			}
			else if((vmarea->vma_start >= lopage) && (vmarea->vma_end <= end))
			{
				dbg(DBG_PRINT,"\n GRADING3D vmmap_remove : Case 4.\n");
				mmobj_t *mmobj = vmarea->vma_obj;
				mmobj->mmo_ops->put(mmobj);
				if(list_link_is_linked( &(vmarea->vma_olink) ))
				{
					dbg(DBG_PRINT,"\n GRADING3D vmmap_remove : Case 4. calling list_remove \n");
					list_remove(&(vmarea->vma_olink));
				}
				list_remove(&(vmarea->vma_plink));
		  		vmarea_free(vmarea);
				continue;
			}
			else
			{
				dbg(DBG_PRINT,"\n GRADING3D vmmap_remove : Case else .\n");
				continue;
			}

		}list_iterate_end();
	}

	dbg(DBG_PRINT,"\n GRADING3D vmmap_remove : returning from function.\n");
	return 0;
}

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int
vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
	/* NOT_YET_IMPLEMENTED("VM: vmmap_is_range_empty"); */

	uint32_t endvfn = startvfn + npages;

	KASSERT((startvfn < endvfn) && (ADDR_TO_PN(USER_MEM_LOW) <= startvfn) && (ADDR_TO_PN(USER_MEM_HIGH) >= endvfn));
	dbg(DBG_PRINT,"\n GRADING3A 3.e KASSERT((startvfn < endvfn) && (ADDR_TO_PN(USER_MEM_LOW) <= startvfn) && (ADDR_TO_PN(USER_MEM_HIGH) >= endvfn)); SUCCESSFUL \n");
	
	vmarea_t *vmarea;
	int range = 0;

	if(!(list_empty(&(map->vmm_list))))
	{
		dbg(DBG_PRINT, "\n GRADING3D vmmap_is_range_empty : list is not empty \n");
		list_iterate_begin( &(map->vmm_list), vmarea, vmarea_t, vma_plink)
		{
			if( (vmarea->vma_start >= endvfn) || (vmarea->vma_end <= startvfn) )
			{
				dbg(DBG_PRINT, "\n GRADING3D vmmap_is_range_empty : range = 1 \n");
				range = 1;
			}
			else
			{
				dbg(DBG_PRINT,"\n GRADING3D vmmap_is_range_empty : Returning zero.\n");
				return 0;
			}
		}list_iterate_end();
	}
	else
	{
		dbg(DBG_PRINT, "\n GRADING3D vmmap_is_range_empty : else part range = 1 \n");
		range = 1;
	}

	dbg(DBG_PRINT, "\n GRADING3D vmmap_is_range_empty : returning range value \n");
	return range;
}

/* Read into 'buf' from the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do so, you will want to find the vmareas
 * to read from, then find the pframes within those vmareas corresponding
 * to the virtual addresses you want to read, and then read from the
 * physical memory that pframe points to. You should not check permissions
 * of the areas. Assume (KASSERT) that all the areas you are accessing exist.
 * Returns 0 on success, -errno on error.
 */
int
vmmap_read(vmmap_t *map, const void *vaddr, void *buf, size_t count)
{
        /* NOT_YET_IMPLEMENTED("VM: vmmap_read"); */

	/*
        Note here: The diagram specifies that vaddr addresse is just the starting addresse of the vmarea and not the page frame vmarea is residing.
        So, we need to find the correct page frame,give the offset (here offset is the page start - vaddr). this offset also needs to be added in the 
        physical pageframe.There is another offset which tell the mmobj from which pageframe you need to start searching for the vmarea in our case.
        And then there is another offset which is part of the page.
    	*/
	vmarea_t *vmarea;
	pframe_t *frame;
	size_t read_size = 0;
	char *buffer = (char*)buf;
	uint32_t v_page = ADDR_TO_PN(vaddr);
	char *physical_page_pointer = NULL;

	if(map == NULL)
	{
		dbg(DBG_PRINT, "\n GRADING3D vmmap_read : empty map. NULL value returning. Exiting\n");
		return NULL;
	}

	if( list_empty( &map->vmm_list ) )
	{
		dbg(DBG_PRINT, "\n GRADING3D vmmap_read : list is empty\n");
	}

	if(!(list_empty(&(map->vmm_list))))
	{
		dbg(DBG_PRINT, "\n GRADING3D vmmap_read : list is not empty\n");
        
		uint32_t page_off = PAGE_OFFSET(vaddr);
		uint32_t p_page = 0;
		uint32_t new_addr = (uint32_t)vaddr;
		char *buffer = (char*)buf;

		while(count > 0)
		{
			/* Getting the VMarea . As explained above, vaddr is just the starting address of the vmarea and not the address of the pageframe.*/
			vmarea = vmmap_lookup(map, v_page);
			if(vmarea == NULL)
			{
		        	dbg(DBG_PRINT,"\n GRADING3D vmmap_read : Vmarea not found, returning the fault.\n");
		        	return -EFAULT;
			}
			/*Physcial page can be found after going to mmobj offset ,then the start of vmarea pageframe, then vmarea offset.
			Getting the Pframe. We can use both Pframe_lookup and pframe_get. I dont' see any difference.*/

			/*page_no = ADDR_TO_PN(vaddr)-(vmarea->vma_start)+(vmarea->vm_off); */
			p_page = v_page - (vmarea->vma_start) + (vmarea->vma_off);
			pframe_lookup( vmarea->vma_obj, p_page, 0, &frame) ;
			if(frame == NULL)
			{
		        	dbg(DBG_PRINT,"\n GRADING3D vmmap_read : Pframe not found, returning the fault.\n");
		        	return -EFAULT;
			}
			physical_page_pointer = (char*)frame->pf_addr+page_off;
			read_size = count;
			memcpy(buffer, physical_page_pointer, read_size);
			count -= read_size;
			physical_page_pointer += read_size;
			new_addr += read_size;
			page_off = PAGE_OFFSET(new_addr);
			v_page = ADDR_TO_PN(new_addr);
		}
	}
	else
	{
		dbg(DBG_PRINT,"\n GRADING3D vmmap_read : returning the Efault.\n");
		return -EFAULT;
	}

	dbg(DBG_PRINT,"\n GRADING3D vmmap_read : returning from function.\n");
	return 0;
}

/* Write from 'buf' into the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do this, you will need to find the correct
 * vmareas to write into, then find the correct pframes within those vmareas,
 * and finally write into the physical addresses that those pframes correspond
 * to. You should not check permissions of the areas you use. Assume (KASSERT)
 * that all the areas you are accessing exist. Remember to dirty pages!
 * Returns 0 on success, -errno on error.
 */
int
vmmap_write(vmmap_t *map, void *vaddr, const void *buf, size_t count)
{
        /* NOT_YET_IMPLEMENTED("VM: vmmap_write"); */

	vmarea_t *vmarea;
	pframe_t *frame;
	size_t write_size = 0;
	char *buffer=(char*)buf;
	uint32_t v_page=ADDR_TO_PN(vaddr);
	char *physical_page_pointer=NULL;
	if(map == NULL)
	{
		dbg(DBG_PRINT, "\n GRADING3D vmmap_write : NULL value returning. Exiting\n");
		return NULL;
	}

	if( list_empty( &map->vmm_list ) )
	{
        	dbg(DBG_PRINT, "\n GRADING3D vmmap_write : empty list \n");
	}

	if(!(list_empty(&(map->vmm_list))))
	{
        
		uint32_t page_off = PAGE_OFFSET(vaddr);
		uint32_t p_page = 0;
		uint32_t new_addr = (uint32_t)vaddr;
		char *buffer = (char*)buf;

		while(count > 0)
		{
			/* Getting the VMarea . As explained above, vaddr is just the starting address of the vmarea and not the address of the pageframe.*/
			vmarea = vmmap_lookup(map,v_page);
			if(vmarea == NULL)
			{
				dbg(DBG_PRINT,"\n GRADING3D vmmap_write : Vmarea not found, returning the fault.\n");
				return -EFAULT;
			}
			/*Physcial page can be found after going to mmobj offset ,then the start of vmarea pageframe, then vmarea offset.
			Getting the Pframe. We can use both Pframe_lookup and pframe_get. I dont' see any difference.*/

			/*page_no = ADDR_TO_PN(vaddr)-(vmarea->vma_start)+(vmarea->vm_off); */
			p_page = v_page - (vmarea->vma_start) + (vmarea->vma_off);
			pframe_lookup( vmarea->vma_obj, p_page, 1, &frame) ;
			if(frame == NULL)
			{
				dbg(DBG_PRINT,"\n GRADING3D vmmap_write : Pframe not found, returning the fault.\n");
				return -EFAULT;
			}

			physical_page_pointer = (char*)frame->pf_addr+page_off;
			write_size = count;
			memcpy(physical_page_pointer, buffer, write_size);
			count -= write_size;
			physical_page_pointer += write_size;
			new_addr += write_size;
			page_off = PAGE_OFFSET(new_addr);
			v_page = ADDR_TO_PN(new_addr);
			pframe_dirty(frame);
		}
	}
	else
	{
		dbg(DBG_PRINT,"\n GRADING3D vmmap_write : returning the Efault.\n");
		return -EFAULT;
	}

	dbg(DBG_PRINT,"\n GRADING3D vmmap_write : returning from function.\n");
	return 0;
}
