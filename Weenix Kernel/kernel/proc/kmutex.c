

#include "globals.h"
#include "errno.h"

#include "util/debug.h"

#include "proc/kthread.h"
#include "proc/kmutex.h"

/*
 * IMPORTANT: Mutexes can _NEVER_ be locked or unlocked from an
 * interrupt context. Mutexes are _ONLY_ lock or unlocked from a
 * thread context.
 */

void
kmutex_init(kmutex_t *mtx)
{
	/* dbg(DBG_PRINT,"\n\n @@@@@@@@@@@@@@@@@@@@@@   204 M     @@@@@@@@@@@@@@@@@@@@@@@ \n\n"); */
	dbg(DBG_PRINT,"\n\n GRADING1C 7 Faber Test : show race test");
	mtx->km_holder = NULL;
	sched_queue_init(&(mtx->km_waitq));
}

/*
 * This should block the current thread (by sleeping on the mutex's
 * wait queue) if the mutex is already taken.
 *
 * No thread should ever try to lock a mutex it already has locked.
 */
void
kmutex_lock(kmutex_t *mtx)
{
	KASSERT(curthr && (curthr != mtx->km_holder));
	dbg(DBG_PRINT,"(GRADING1A 5.a) KASSERT(curthr && (curthr != mtx->km_holder)) is success\n");
	/* kthread_t *x= mtx->km_holder; */
	if(mtx->km_holder == NULL)
	{
		mtx->km_holder = curthr;
		/*dbg(DBG_PRINT,"\n\n @@@@@@@@@@@@@@@@@    205 M     @@@@@@@@@@@@@@@@@@ \n\n");*/
		dbg(DBG_PRINT,"\n\n GRADING1C 7 Faber test : show race test");
	}
	else
	{
  		sched_sleep_on(&(mtx->km_waitq));
  		/*dbg(DBG_PRINT,"\n\n @@@@@@@@@@@@@@@@@@    206 M      @@@@@@@@@@@@@@@@@@@@@@@@@@ \n\n");*/
  		dbg(DBG_PRINT,"\n\n GRADING1C 7 Faber Test: show race test");
	}
}

/*
 * This should do the same as kmutex_lock, but use a cancellable sleep
 * instead.
 */
int
kmutex_lock_cancellable(kmutex_t *mtx)
{
	KASSERT(curthr && (curthr != mtx->km_holder));
	dbg(DBG_PRINT,"(GRADING1A 5.b) KASSERT(curthr && (curthr != mtx->km_holder)) is success\n");
	dbg(DBG_TEST,"(GRADING1A 5.b) KASSERT(curthr && (curthr != mtx->km_holder)) is success\n");
	/* kthread_t *y = mtx->km_holder; */
	if(mtx->km_holder == NULL)
	{
		mtx->km_holder=curthr;
		/*dbg(DBG_PRINT,"\n\n @@@@@@@@@@@@@@@@@@@  207 M    @@@@@@@@@@@@@@@@@@@@@@@@ \n\n");*/
		dbg(DBG_PRINT,"\n\n GRADING1C 7 Faber Test : show race test");

		return 0;
	}
	if(sched_cancellable_sleep_on(&(mtx->km_waitq)) == -EINTR)
	{

		/*dbg(DBG_PRINT,"\n\n @@@@@@@@@@@@@@@@@@@@2    208 M      @@@@@@@@@@@@@@@@@@@@@@@@@@ \n\n");*/
		dbg(DBG_PRINT,"\n\n GRADING1C 7 Faber Test : show race test");
		return -EINTR;
	}

	/*dbg(DBG_PRINT,"\n\n @@@@@@@@@@@@@@@@@@@@@@@@@   209 M     @@@@@@@@@@@@@@@@@@@@@@@@@@@@ \n\n");*/
	dbg(DBG_PRINT,"\n\n GRADING1C 7 Faber test : show race test");
	/* mtx->km_holder=curthr; */
	return 0;
}

/*
 * If there are any threads waiting to take a lock on the mutex, one
 * should be woken up and given the lock.
 *
 * Note: This should _NOT_ be a blocking operation!
 *
 * Note: Don't forget to add the new owner of the mutex back to the
 * run queue.
 *
 * Note: Make sure that the thread on the head of the mutex's wait
 * queue becomes the new owner of the mutex.
 *
 * @param mtx the mutex to unlock
 */
void
kmutex_unlock(kmutex_t *mtx)
{
	KASSERT(curthr && (curthr == mtx->km_holder));
	dbg(DBG_PRINT,"(GRADING1A 5.c) KASSERT(curthr && (curthr == mtx->km_holder)) is success \n");
	kthread_t *next_holder = NULL;
	next_holder=sched_wakeup_on(&(mtx->km_waitq));
	mtx->km_holder = next_holder;
	KASSERT(curthr != mtx->km_holder);
	dbg(DBG_PRINT,"(GRADING1A 5.c) KASSERT(curthr != mtx->km_holder) is success \n");

	/*dbg(DBG_PRINT,"\n\n @@@@@@@@@@@@@@@@     210 M      @@@@@@@@@@@@@@@@@@@@@@@@@@ \n\n");*/
	/* dbg(DBG_PRINT,"\n\n GRADING1C 7 Faber Test : show race test"); */

}

