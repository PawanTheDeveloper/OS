

#include "kernel.h"
#include "config.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "proc/kthread.h"
#include "proc/proc.h"
#include "proc/sched.h"
#include "proc/proc.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/mm.h"
#include "mm/mman.h"

#include "vm/vmmap.h"

#include "fs/vfs.h"
#include "fs/vfs_syscall.h"
#include "fs/vnode.h"
#include "fs/file.h"

proc_t *curproc = NULL; /* global */
static slab_allocator_t *proc_allocator = NULL;

static list_t _proc_list;
static proc_t *proc_initproc = NULL; /* Pointer to the init process (PID 1) */

void
proc_init()
{
        list_init(&_proc_list);
        proc_allocator = slab_allocator_create("proc", sizeof(proc_t));
        KASSERT(proc_allocator != NULL);
}

proc_t *
proc_lookup(int pid)
{
        proc_t *p;
        list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                if (p->p_pid == pid) {
                        return p;
                }
        } list_iterate_end();
        return NULL;
}

list_t *
proc_list()
{
        return &_proc_list;
}

static pid_t next_pid = 0;

/**
 * Returns the next available PID.
 *
 * Note: Where n is the number of running processes, this algorithm is
 * worst case O(n^2). As long as PIDs never wrap around it is O(n).
 *
 * @return the next available PID
 */
static int
_proc_getid()
{
        proc_t *p;
        pid_t pid = next_pid;
        while (1) {
failed:
                list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                        if (p->p_pid == pid)
                        {
                                if ((pid = (pid + 1) % PROC_MAX_COUNT) == next_pid)
                                {
                                        return -1;
                                }
                                else {
                                        goto failed;
                                }
                        }
                } list_iterate_end();
                next_pid = (pid + 1) % PROC_MAX_COUNT;
                return pid;
        }
}

/*
 * The new process, although it isn't really running since it has no
 * threads, should be in the PROC_RUNNING state.
 *
 * Don't forget to set proc_initproc when you create the init
 * process. You will need to be able to reference the init process
 * when reparenting processes to the init process.
 */
proc_t *
proc_create(char *name)
{
	/*dbg(DBG_PRINT,"\n\n################################################      300        ###############################################################\n\n");*/
	int i=0;
	/*Create a new process using slab allocator.*/
	proc_t *new_process=(proc_t *)slab_obj_alloc(proc_allocator);
	KASSERT(new_process!=NULL);

	/*Proc Create structure, Set below all the possible values of the struct proc---------------------------------->*/
	/*1. pid_t           p_pid;                 our pid */
	new_process->p_pid = _proc_getid();
	KASSERT(PID_IDLE != new_process->p_pid || list_empty(&_proc_list));
	dbg(DBG_PRINT, "\n\n  GRADING1A 2.a) KASSERT (PID_IDLE !=new_process->p_pid || list_empty(&_proc_list))  is checking successfully." );
	/*Check the value of pid and make the adjustment.*/

	if(new_process->p_pid == PID_INIT)
	{
		KASSERT(PID_INIT != new_process->p_pid || PID_IDLE == curproc->p_pid);
		dbg ( DBG_PRINT, "\n\n  GRADING1A 2.a) KASSERT (PID_INIT !=new_process->p_pid) is checking successfully." );
		/*dbg(DBG_PRINT,"\n\n################################################      301        ###############################################################\n\n");*/
		dbg(DBG_PRINT,"\n\nGRADING1D 1.Sunghan-Test");
		proc_initproc = new_process;
	}
	/* 2.char            p_comm[PROC_NAME_LEN];  process name */
	KASSERT(name != NULL);
	strncpy(new_process->p_comm, name, PROC_NAME_LEN);

	/*3.list_t          p_threads;        the process's thread list */
	list_init(&(new_process->p_threads));

	/*4. list_t          p_children;       the process's children list */
	list_init(&(new_process->p_children));

	/* 5. struct proc    *p_pproc;          our parent process */
	/*Hook it with the current process.*/

	if((new_process->p_pid != PID_IDLE))
	{
		/*dbg(DBG_PRINT,"\n\n################################################      302        ###############################################################\n\n");*/
		dbg(DBG_PRINT,"\n\nGRADING1C 1.Faber_test.Waitpid Test.");

		new_process->p_pproc = curproc;
	}

	/*6. int             p_status;        exit status */
	new_process->p_status = 0;

	/*7. int             p_state;          running/sleeping/etc. */
	new_process->p_state = PROC_RUNNING;

	/*8. ktqueue_t       p_wait;           queue for wait(2) */
	sched_queue_init(&(new_process->p_wait));

	/*9. pagedir_t      *p_pagedir;*/
	new_process->p_pagedir = pt_create_pagedir();

	/*10. list_link_t     p_list_link;      link on the list of all processes */
	list_link_init(&(new_process->p_list_link));

	/*11. list_link_t     p_child_link;     link on proc list of children */
	list_link_init(&(new_process->p_child_link));
	/*------------------------------------------------------------------------------------------------------------------->*/
	/*Now updating all the lists and queues , informing that a new process have been created.
	1. If curProc exists then, update its child list.*/
	if(curproc != NULL)
	{
		/*dbg(DBG_PRINT,"\n\n################################################      303        ###############################################################\n\n");*/
		dbg(DBG_PRINT,"\n\nGRADING1C 1.Faber_test.Waitpid Test.");
		list_insert_tail(&curproc->p_children, &new_process->p_child_link);
	}
	/*2. Add this newly created process inside the global proc_list.*/

	dbg(DBG_PRINT, "\n\nGRADING1C 1.Faber Test : Process created successfully.");
	list_insert_tail(&_proc_list,&(new_process->p_list_link));

	for(i=0; i<NFILES; ++i)
	{
		new_process->p_files[i] = NULL;
	}
	if(curproc != NULL)
	{
		new_process->p_cwd = curproc->p_cwd;
		if(curproc->p_cwd != NULL)
		{
			vref(curproc->p_cwd);
		}
	}
	else
	{
		new_process->p_cwd = NULL;
	}

	/* Specific to VM */

	new_process->p_brk = NULL;
	new_process->p_start_brk = NULL;

	new_process->p_vmmap = vmmap_create();
	new_process->p_vmmap->vmm_proc = new_process;

	return new_process;
}

/**
 * Cleans up as much as the process as can be done from within the
 * process. This involves:
 *    - Closing all open files (VFS)
 *    - Cleaning up VM mappings (VM)
 *    - Waking up its parent if it is waiting
 *    - Reparenting any children to the init process
 *    - Setting its status and state appropriately
 *
 * The parent will finish destroying the process within do_waitpid (make
 * sure you understand why it cannot be done here). Until the parent
 * finishes destroying it, the process is informally called a 'zombie'
 * process.
 *
 * This is also where any children of the current process should be
 * reparented to the init process (unless, of course, the current
 * process is the init process. However, the init process should not
 * have any children at the time it exits).
 *
 * Note: You do _NOT_ have to special case the idle process. It should
 * never exit this way.
 *
 * @param status the status to exit the process with
 */
void
proc_cleanup(int status)
{

	int i=0;
	/* NOT_YET_IMPLEMENTED("PROCS: proc_cleanup");
	1. Status and State.*/
	curproc->p_status = status;
	curproc->p_state = PROC_DEAD;
	/*2. Reparenting all the children.
	 Handle init case*/
	if(curproc->p_cwd != NULL)
	{
			vput(curproc->p_cwd);
	}
	KASSERT(NULL != proc_initproc);
	dbg ( DBG_PRINT, "\n\n  GRADING1A 2.b) KASSERT (NULL != proc_initproc) is checking successfully." );

	KASSERT(1 <= curproc->p_pid);
	dbg ( DBG_PRINT, "\n\n  GRADING1A 2.b) KASSERT (1 <= curproc->p_pid) is checking successfully." );

	KASSERT(NULL != curproc->p_pproc);
	dbg ( DBG_PRINT, "\n\n  GRADING1A 2.b) KASSERT (NULL != curproc->p_pproc) is checking successfully(PRECONDTION)." );

	sched_wakeup_on(&curproc->p_pproc->p_wait);

	if ((curproc->p_pid != PID_INIT) && (list_empty(&(curproc->p_children))!=1))
	{
		/*dbg(DBG_PRINT,"\n\n################################################      331        ###############################################################\n\n");*/

		dbg(DBG_PRINT,"\n\nGRADING1C 6.Faber Test,Reparenting_test.");

	    	proc_t *temp=NULL;
	    	list_iterate_begin(&curproc->p_children, temp, proc_t, p_child_link)
	    	{
	      		list_remove(&temp->p_child_link);
	      		list_insert_tail(&proc_initproc->p_children, &temp->p_child_link);
	      		temp->p_pproc=proc_initproc;
	    	}list_iterate_end();
	}
	/*3. Waking up its parent.*/
	KASSERT(NULL != curproc->p_pproc);
	dbg ( DBG_PRINT, "\n\n  GRADING1A 2.b) KASSERT (NULL != curproc->p_pproc) is checking successfully(POSTCONDITION)." );
	/*dbg(DBG_PRINT,"\n\n################################################      322E        ###############################################################\n\n");*/
	for (i = 0; i < NFILES; ++i)
	{
		if (curproc->p_files[i])
			do_close(i);
	}

	/* Specific to VM */

	if ( curproc->p_vmmap != NULL )
	{
		vmmap_destroy(curproc->p_vmmap);
	}
}

/*
 * This has nothing to do with signals and kill(1).
 *
 * Calling this on the current process is equivalent to calling
 * do_exit().
 *
 * In Weenix, this is only called from proc_kill_all.
 */
void
proc_kill(proc_t *p, int status)
{
	/*dbg(DBG_PRINT,"\n\n################################################      330        ###############################################################\n\n");*/

	if(p == curproc)
	{
		/*dbg(DBG_PRINT,"\n\n################################################      331        ###############################################################\n\n");*/
		dbg(DBG_PRINT,"\n\nGRADING1C 8.Faber Test, Proc Kill test.");
		do_exit(status);
	}
	else
	{
		dbg(DBG_PRINT,"\n\nGRADING1C 8.Faber Test, Proc Kill test.");

		/*dbg(DBG_PRINT,"\n\n################################################      332        ###############################################################\n\n");*/

		kthread_t *temp;
		list_iterate_begin(&p->p_threads,temp,kthread_t,kt_plink)
		{
		      kthread_cancel(temp, (void *)status);
		}list_iterate_end();

		p->p_status = status;
	}
	/*dbg(DBG_PRINT,"\n\n################################################      333E        ###############################################################\n\n");*/

}

/*
 * Remember, proc_kill on the current process will _NOT_ return.
 * Don't kill direct children of the idle process.
 *
 * In Weenix, this is only called by sys_halt.
 */
void
proc_kill_all()
{
	/*dbg(DBG_PRINT,"\n\n################################################      340        ###############################################################\n\n");*/
	/*	dbg(DBG_PRINT,"\n\nEntered proc kill all");*/
	     /*NOT_YET_IMPLEMENTED("PROCS: proc_kill_all");
		If it is the first init process, exit the program.*/
		/*Iterate over killing all children except DON'T KILL DIRECT CHILDREN OF THE IDLE PROCESS*/
		proc_t *temp;
	    	list_iterate_begin(&_proc_list, temp, proc_t, p_list_link)
		{
			if(temp->p_pid == PID_IDLE)
			{
				dbg(DBG_PRINT,"\n\nGRADING1C 8.Faber Test, Proc Kill test.process is PID_IDLE");


				/*dbg(DBG_PRINT,"\n\n################################################      342        ###############################################################\n\n");*/

				/*Don't do anything, just continue;*/
				continue;
			}

			if(temp->p_pproc->p_pid != PID_IDLE && temp != curproc && temp->p_pid != PID_IDLE)
			{
				dbg(DBG_PRINT,"\n\nGRADING1C 8.Faber Test, Proc Kill test.");

				/*dbg(DBG_PRINT,"\n\n################################################      344        ###############################################################\n\n");*/

				proc_kill(temp, temp->p_status);
			}
		}list_iterate_end();
		/*dbg(DBG_PRINT,"\n\n################################################      345E        ###############################################################\n\n");*/

		dbg(DBG_PRINT, "GRADING1C 8 Faber Test : proc kill test");

		do_exit(curproc->p_status);
}
/*
 * This function is only called from kthread_exit.
 *
 * Unless you are implementing MTP, this just means that the process
 * needs to be cleaned up and a new thread needs to be scheduled to
 * run. If you are implementing MTP, a single thread exiting does not
 * necessarily mean that the process should be exited.
 */
void
proc_thread_exited(void *retval)
{
	/*dbg(DBG_PRINT,"\n\n################################################      350        ###############################################################\n\n");*/

	/*NOT_YET_IMPLEMENTED("PROCS: proc_thread_exited");*/
	curthr->kt_state = KT_EXITED;
	dbg(DBG_PRINT, "Inside proc_thread_exited: %d",(int)retval);
	proc_cleanup((int)retval);

	dbg(DBG_PRINT, "GRADING1C 5 Faber Test : Exit me test");

	sched_switch();
}

/* If pid is -1 dispose of one of the exited children of the current
 * process and return its exit status in the status argument, or if
 * all children of this process are still running, then this function
 * blocks on its own p_wait queue until one exits.
 *
 * If pid is greater than 0 and the given pid is a child of the
 * current process then wait for the given pid to exit and dispose
 * of it.
 *
 * If the current process has no children, or the given pid is not
 * a child of the current process return -ECHILD.
 *
 * Pids other than -1 and positive numbers are not supported.
 * Options other than 0 are not supported.
 */
pid_t
do_waitpid(pid_t pid, int options, int *status)
{
	/*dbg(DBG_PRINT,"\n\n################################################      360        ###############################################################\n\n");*/

		    /*NOT_YET_IMPLEMENTED("PROCS: do_waitpid");*/
			pid_t childpid;

			int child_dead=0;
			int own_child=0;

			proc_t *child_process;
			kthread_t *child_thread;

			KASSERT(NULL != proc_initproc);
			dbg(DBG_PRINT, "(GRADING1A 2.c) INIT Proc is not NULL\n");

			if(pid <-1 || list_empty(&curproc->p_children))
			{
				/*dbg(DBG_PRINT,"\n\n################################################      361        ###############################################################\n\n");*/
				dbg(DBG_PRINT,"\n\nGRADING1C 1.Faber_test.Waitpid Test.");
				return -ECHILD;
			}
			else if(pid==-1)
			{
				dbg(DBG_PRINT,"\n\nGRADING1C 1.Faber_test.Waitpid Any Test. when pid = -1");

				/*dbg(DBG_PRINT,"\n\n################################################      362        ###############################################################\n\n");*/

				while(child_dead==0)
				{
					if(list_empty(&(curproc->p_children)))
					{
						/*dbg(DBG_PRINT,"\n\n################################################      361        ###############################################################\n\n");*/
						dbg(DBG_PRINT,"\n\nGRADING1C 1.Faber_test.Waitpid Test.");

						return -ECHILD;
					}
					list_iterate_begin(&curproc->p_children, child_process, proc_t, p_child_link)
					{
						if(child_process->p_state==PROC_DEAD)
						{
							/*dbg(DBG_PRINT,"\n\n################################################      363        ###############################################################\n\n");*/

							KASSERT(NULL != child_process);
							dbg(DBG_PRINT, "(GRADING1A 2.c) dead child process is not NULL\n");
							dbg(DBG_PRINT,"\n\nGRADING1C 1.Faber_test.Waitpid Test.");


							child_dead=1;
							childpid=child_process->p_pid;
							KASSERT(-1 == pid || childpid == pid); /* should be able to find a valid process ID for the process */
							dbg(DBG_PRINT, "(GRADING1A 2.c) valid process ID found\n");
							/*dbg(DBG_PRINT,"\n\n################################################      365        ###############################################################\n\n");*/
							list_iterate_begin(&child_process->p_threads, child_thread, kthread_t, kt_plink)
							{
								KASSERT(KT_EXITED==child_thread->kt_state);
								dbg(DBG_PRINT, "(GRADING1A 2.c) Current thead doesn't have KT-EXITED state\n");

								kthread_destroy(child_thread);
							}list_iterate_end();
							if(status!=NULL)
							{
								dbg(DBG_PRINT,"\n\nGRADING1C 1.Faber_test.Waitpid Test.");

								/*dbg(DBG_PRINT,"\n\n################################################      374        ###############################################################\n\n");*/
								*status=child_process->p_status;
							}
							list_remove(&child_process->p_child_link);
							list_remove(&child_process->p_list_link);
							KASSERT(NULL != child_process->p_pagedir); 
							dbg(DBG_PRINT, "(GRADING1A 2.c) this process has a valid pagedir\n");
							pt_destroy_pagedir(child_process->p_pagedir);
							slab_obj_free(proc_allocator,(void*)child_process);
							return childpid;
						}
					}list_iterate_end();
					if(child_dead==0)
					{
						dbg(DBG_PRINT,"\n\nGRADING1C 1.Faber_test.Waitpid Test.");

					/*	dbg(DBG_PRINT,"\n\n################################################      364        ###############################################################\n\n");*/

						sched_sleep_on(&curproc->p_wait);
					}
				}
			}
			else
			{
				/*dbg(DBG_PRINT,"\n\n################################################      369        ###############################################################\n\n");*/
				list_iterate_begin(&curproc->p_children, child_process, proc_t, p_child_link)
				{
					if(child_process->p_pid==pid)
					{
						dbg(DBG_PRINT,"\n\nGRADING1C 1.Faber_test.Waitpid Test. when pid is > 0");

						while(child_dead==0)
						{
							if(child_process->p_state==PROC_DEAD)
							{
								dbg(DBG_PRINT,"\n\nGRADING1C 1.Faber_test.Waitpid Test.");

								/*dbg(DBG_PRINT,"\n\n################################################      370        ###############################################################\n\n");*/
								KASSERT(NULL != child_process);
								dbg(DBG_PRINT, "(GRADING1A 2.c) dead child process is not NULL\n");
								child_dead=1;
								childpid=child_process->p_pid;
								KASSERT(-1 == pid || childpid == pid); /* should be able to find a valid process ID for the process */
								dbg(DBG_PRINT, "(GRADING1A 2.c) valid process ID found\n");
								/*dbg(DBG_PRINT,"\n\n################################################      372        ###############################################################\n\n");*/

								list_iterate_begin(&child_process->p_threads, child_thread, kthread_t, kt_plink)
								{
									KASSERT(KT_EXITED==child_thread->kt_state);
									dbg(DBG_PRINT, "(GRADING1A 2.c) Current thead doesn't have KT-EXITED state\n");

									kthread_destroy(child_thread);
								}list_iterate_end();
								if(status!=NULL)
								{
									dbg(DBG_PRINT,"\n\nGRADING1C 1.Faber_test.Waitpid Test.");

									/*dbg(DBG_PRINT,"\n\n################################################      374        ###############################################################\n\n");*/
									*status=child_process->p_status;
								}
								list_remove(&child_process->p_child_link);
								list_remove(&child_process->p_list_link);
								KASSERT(NULL != child_process->p_pagedir);
								dbg(DBG_PRINT, "(GRADING1A 2.c) this process has a valid pagedir\n");
								pt_destroy_pagedir(child_process->p_pagedir);
								slab_obj_free(proc_allocator,(void*)child_process);
								return childpid;

							}
							if(child_dead==0)
							{
								/*dbg(DBG_PRINT,"\n\n################################################      371        ###############################################################\n\n");*/
								dbg(DBG_PRINT,"\n\nGRADING1C 1.Faber_test.Waitpid Test.");

								sched_sleep_on(&curproc->p_wait);
							}
						}
					}
				}list_iterate_end();
			}
			/*dbg(DBG_PRINT,"\n\n################################################      375        ###############################################################\n\n");*/

			dbg(DBG_PRINT, "GRADING1C 1 Faber Test : wait pid test");

			return -ECHILD;
}

/*
 * Cancel all threads, join with them, and exit from the current
 * thread.
 *
 * @param status the exit status of the process
 */
void
do_exit(int status)
{
	dbg(DBG_PRINT,"\n\nGRADING1C 1.Faber_test.Waitpid Test.");

	/*dbg(DBG_PRINT,"\n\n################################################      380        ###############################################################\n\n");*/

	/*NOT_YET_IMPLEMENTED("PROCS: do_exit");*/
	kthread_t *temp;
	list_iterate_begin(&curproc->p_threads, temp, kthread_t, kt_plink)
	{
		kthread_cancel(temp, (void*) status);
	}list_iterate_end();


	kthread_exit((void*)status);
}

size_t
proc_info(const void *arg, char *buf, size_t osize)
{
        const proc_t *p = (proc_t *) arg;
        size_t size = osize;
        proc_t *child;

        KASSERT(NULL != p);
        KASSERT(NULL != buf);

        iprintf(&buf, &size, "pid:          %i\n", p->p_pid);
        iprintf(&buf, &size, "name:         %s\n", p->p_comm);
        if (NULL != p->p_pproc) {
                iprintf(&buf, &size, "parent:       %i (%s)\n",
                        p->p_pproc->p_pid, p->p_pproc->p_comm);
        } else {
                iprintf(&buf, &size, "parent:       -\n");
        }

#ifdef __MTP__
        int count = 0;
        kthread_t *kthr;
        list_iterate_begin(&p->p_threads, kthr, kthread_t, kt_plink) {
                ++count;
        } list_iterate_end();
        iprintf(&buf, &size, "thread count: %i\n", count);
#endif

        if (list_empty(&p->p_children)) {
                iprintf(&buf, &size, "children:     -\n");
        } else {
                iprintf(&buf, &size, "children:\n");
        }
        list_iterate_begin(&p->p_children, child, proc_t, p_child_link) {
                iprintf(&buf, &size, "     %i (%s)\n", child->p_pid, child->p_comm);
        } list_iterate_end();

        iprintf(&buf, &size, "status:       %i\n", p->p_status);
        iprintf(&buf, &size, "state:        %i\n", p->p_state);

#ifdef __VFS__
#ifdef __GETCWD__
        if (NULL != p->p_cwd) {
                char cwd[256];
                lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
                iprintf(&buf, &size, "cwd:          %-s\n", cwd);
        } else {
                iprintf(&buf, &size, "cwd:          -\n");
        }
#endif /* __GETCWD__ */
#endif

#ifdef __VM__
        iprintf(&buf, &size, "start brk:    0x%p\n", p->p_start_brk);
        iprintf(&buf, &size, "brk:          0x%p\n", p->p_brk);
#endif

        return size;
}

size_t
proc_list_info(const void *arg, char *buf, size_t osize)
{
        size_t size = osize;
        proc_t *p;

        KASSERT(NULL == arg);
        KASSERT(NULL != buf);

#if defined(__VFS__) && defined(__GETCWD__)
        iprintf(&buf, &size, "%5s %-13s %-18s %-s\n", "PID", "NAME", "PARENT", "CWD");
#else
        iprintf(&buf, &size, "%5s %-13s %-s\n", "PID", "NAME", "PARENT");
#endif

        list_iterate_begin(&_proc_list, p, proc_t, p_list_link) {
                char parent[64];
                if (NULL != p->p_pproc) {
                        snprintf(parent, sizeof(parent),
                                 "%3i (%s)", p->p_pproc->p_pid, p->p_pproc->p_comm);
                } else {
                        snprintf(parent, sizeof(parent), "  -");
                }

#if defined(__VFS__) && defined(__GETCWD__)
                if (NULL != p->p_cwd) {
                        char cwd[256];
                        lookup_dirpath(p->p_cwd, cwd, sizeof(cwd));
                        iprintf(&buf, &size, " %3i  %-13s %-18s %-s\n",
                                p->p_pid, p->p_comm, parent, cwd);
                } else {
                        iprintf(&buf, &size, " %3i  %-13s %-18s -\n",
                                p->p_pid, p->p_comm, parent);
                }
#else
                iprintf(&buf, &size, " %3i  %-13s %-s\n",
                        p->p_pid, p->p_comm, parent);
#endif
        } list_iterate_end();
        return size;
}
