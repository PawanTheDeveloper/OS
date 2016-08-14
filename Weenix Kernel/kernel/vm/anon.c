

#include "globals.h"
#include "errno.h"

#include "util/string.h"
#include "util/debug.h"

#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "mm/slab.h"
#include "mm/tlb.h"

int anon_count = 0; /* for debugging/verification purposes */

static slab_allocator_t *anon_allocator;

static void anon_ref(mmobj_t *o);
static void anon_put(mmobj_t *o);
static int  anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf);
static int  anon_fillpage(mmobj_t *o, pframe_t *pf);
static int  anon_dirtypage(mmobj_t *o, pframe_t *pf);
static int  anon_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t anon_mmobj_ops = {
        .ref = anon_ref,
        .put = anon_put,
        .lookuppage = anon_lookuppage,
        .fillpage  = anon_fillpage,
        .dirtypage = anon_dirtypage,
        .cleanpage = anon_cleanpage
};

/*
 * This function is called at boot time to initialize the
 * anonymous page sub system. Currently it only initializes the
 * anon_allocator object.
 */
void
anon_init()
{
        /* NOT_YET_IMPLEMENTED("VM: anon_init"); */

	anon_allocator = slab_allocator_create("anonymous", sizeof(mmobj_t));

	KASSERT(anon_allocator);
	dbg(DBG_PRINT, "\n GRADING3A 4.a KASSERT(anon_allocator) is successful. \n");
}

/*
 * You'll want to use the anon_allocator to allocate the mmobj to
 * return, then then initialize it. Take a look in mm/mmobj.h for
 * macros which can be of use here. Make sure your initial
 * reference count is correct.
 */
mmobj_t *
anon_create()
{
        /* NOT_YET_IMPLEMENTED("VM: anon_create"); */

	dbg( DBG_PRINT, "\n GRADING3D ENTERING anon_create() \n");

	mmobj_t *mmobj = (mmobj_t*)slab_obj_alloc(anon_allocator);
	if (mmobj != NULL)
	{
		dbg( DBG_PRINT, "\n GRADING3D INSIDE anon_create() : mmobj is not null \n");
		mmobj_init(mmobj, &anon_mmobj_ops);
		mmobj->mmo_refcount = 1;

	}

	dbg( DBG_PRINT, "\n GRADING3D EXITING anon_create() \n");
	return mmobj;
}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void
anon_ref(mmobj_t *o)
{
        /* NOT_YET_IMPLEMENTED("VM: anon_ref"); */

	KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
	dbg(DBG_PRINT, "\n GRADING3A 4.b KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops)) is successful. ");

	o->mmo_refcount++;
}

/*
 * Decrement the reference count on the object. If, however, the
 * reference count on the object reaches the number of resident
 * pages of the object, we can conclude that the object is no
 * longer in use and, since it is an anonymous object, it will
 * never be used again. You should unpin and uncache all of the
 * object's pages and then free the object itself.
 */
static void
anon_put(mmobj_t *o)
{
        /* NOT_YET_IMPLEMENTED("VM: anon_put"); */

	KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
	dbg(DBG_PRINT, "\n GRADING3A 4.c KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops)) is successful. \n");

	if ( (o->mmo_refcount - 1) <= o->mmo_nrespages )
	{
		dbg( DBG_PRINT, "\n GRADING3D INSIDE anon_put() : refcount - 1 <= no. of res pages \n");
		if ( ! list_empty ( &o->mmo_respages ) )
		{
			dbg( DBG_PRINT, "\n GRADING3D INSIDE anon_put() : list is not empty \n");
			pframe_t *pf;
			list_iterate_begin ( &o->mmo_respages, pf, pframe_t, pf_olink )
			{
				if ( pframe_is_pinned (pf) )
				{
					dbg( DBG_PRINT, "\n GRADING3D INSIDE anon_put() : pframe is pinned then unpin \n");
					pframe_unpin(pf);
				}

				if ( pframe_is_busy (pf) )
				{
					dbg( DBG_PRINT, "\n GRADING3D INSIDE anon_put() : pframe is busy then sleep \n");
					sched_sleep_on( &pf->pf_waitq );
				}
				if ( pframe_is_dirty (pf) )
				{
					dbg( DBG_PRINT, "\n GRADING3D INSIDE anon_put() : pframe is dirty then clean \n");
					pframe_clean(pf);
				}

				pframe_free(pf);

			} list_iterate_end();

			slab_obj_free(anon_allocator, o);
		}
	}

	o->mmo_refcount--;
	dbg( DBG_PRINT, "\n GRADING3D EXITING anon_put() \n");
}

/* Get the corresponding page from the mmobj. No special handling is
 * required. */
static int
anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{
        /* NOT_YET_IMPLEMENTED("VM: anon_lookuppage"); */

	dbg( DBG_PRINT, "\n GRADING3D ENTERING anon_lookuppage() \n");

	int pfrm = pframe_get( o, pagenum, pf);

	if ( pfrm >= 0 )
	{
		dbg( DBG_PRINT, "\n GRADING3D INSIDE anon_lookuppage() : pframe >= 0 \n");
		return pfrm;
	}

	dbg( DBG_PRINT, "\n GRADING3D EXITING anon_lookuppage() \n");
	return -1;
}

/* The following three functions should not be difficult. */

static int
anon_fillpage(mmobj_t *o, pframe_t *pf)
{
        /* NOT_YET_IMPLEMENTED("VM: anon_fillpage"); */

	KASSERT(pframe_is_busy(pf));
	dbg(DBG_PRINT, "\n GRADING 3A 4.d KASSERT(pframe_is_busy(pf)) is successful. \n");

	KASSERT(!pframe_is_pinned(pf));
	dbg(DBG_PRINT, "\n GRADING 3A 4.d KASSERT(!pframe_is_pinned(pf)) is successful. \n");

	memset(pf->pf_addr, 0, PAGE_SIZE);
	if(!pframe_is_pinned(pf))
	{
		dbg( DBG_PRINT, "\n GRADING3D INSIDE anon_fillpage() : pframe is not pinned then pin it \n");
		pframe_pin(pf);
	}
	dbg( DBG_PRINT, "\n GRADING3D EXITING anon_fillpage() \n");
        return 0;
}

static int
anon_dirtypage(mmobj_t *o, pframe_t *pf)
{
        /* NOT_YET_IMPLEMENTED("VM: anon_dirtypage"); */
        return -1;
}

static int
anon_cleanpage(mmobj_t *o, pframe_t *pf)
{
        /* NOT_YET_IMPLEMENTED("VM: anon_cleanpage"); */
        return -1;
}
