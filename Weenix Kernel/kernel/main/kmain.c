

#include "types.h"
#include "globals.h"
#include "kernel.h"

#include "util/gdb.h"
#include "util/init.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/printf.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/pagetable.h"
#include "mm/pframe.h"

#include "vm/vmmap.h"
#include "vm/shadowd.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "main/acpi.h"
#include "main/apic.h"
#include "main/interrupt.h"
#include "main/gdt.h"

#include "proc/sched.h"
#include "proc/proc.h"
#include "proc/kthread.h"

#include "drivers/dev.h"
#include "drivers/blockdev.h"
#include "drivers/disk/ata.h"
#include "drivers/tty/virtterm.h"
#include "drivers/pci.h"

#include "api/exec.h"
#include "api/syscall.h"

#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/fcntl.h"
#include "fs/stat.h"

#include "test/kshell/kshell.h"
#include "errno.h"

GDB_DEFINE_HOOK(boot)
GDB_DEFINE_HOOK(initialized)
GDB_DEFINE_HOOK(shutdown)

static void       hard_shutdown(void);
static void      *bootstrap(int arg1, void *arg2);
static void      *idleproc_run(int arg1, void *arg2);
static kthread_t *initproc_create(void);
static void      *initproc_run(int arg1, void *arg2);

static context_t bootstrap_context;
extern int gdb_wait;

extern void *sunghan_test(int, void*);
extern void *sunghan_deadlock_test(int, void*);
extern void *faber_thread_test(int, void*);
extern void *vfstest_main(int, void*);
extern int faber_fs_thread_test(kshell_t *ksh, int argc, char **argv);
extern int faber_directory_test(kshell_t *ksh, int argc, char **argv);
int faber_test_process(kshell_t *, int, char **);
int sunghan_test_process(kshell_t *, int, char **);
int sunghanD_test_process(kshell_t *, int, char **);
int vfs_test_process(kshell_t *, int, char **);



/**
 * This is the first real C function ever called. It performs a lot of
 * hardware-specific initialization, then creates a pseudo-context to
 * execute the bootstrap function in.
 */
void
kmain()
{
        GDB_CALL_HOOK(boot);

        dbg_init();
        dbgq(DBG_CORE, "Kernel binary:\n");
        dbgq(DBG_CORE, "  text: 0x%p-0x%p\n", &kernel_start_text, &kernel_end_text);
        dbgq(DBG_CORE, "  data: 0x%p-0x%p\n", &kernel_start_data, &kernel_end_data);
        dbgq(DBG_CORE, "  bss:  0x%p-0x%p\n", &kernel_start_bss, &kernel_end_bss);

        page_init();

        pt_init();
        slab_init();
        pframe_init();

        acpi_init();
        apic_init();
        pci_init();
        intr_init();

        gdt_init();

        /* initialize slab allocators */
#ifdef __VM__
        anon_init();
        shadow_init();
#endif
        vmmap_init();
        proc_init();
        kthread_init();

#ifdef __DRIVERS__
        bytedev_init();
        blockdev_init();
#endif

        void *bstack = page_alloc();
        pagedir_t *bpdir = pt_get();
        KASSERT(NULL != bstack && "Ran out of memory while booting.");
        /* This little loop gives gdb a place to synch up with weenix.  In the
         * past the weenix command started qemu was started with -S which
         * allowed gdb to connect and start before the boot loader ran, but
         * since then a bug has appeared where breakpoints fail if gdb connects
         * before the boot loader runs.  See
         *
         * https://bugs.launchpad.net/qemu/+bug/526653
         *
         * This loop (along with an additional command in init.gdb setting
         * gdb_wait to 0) sticks weenix at a known place so gdb can join a
         * running weenix, set gdb_wait to zero  and catch the breakpoint in
         * bootstrap below.  See Config.mk for how to set GDBWAIT correctly.
         *
         * DANGER: if GDBWAIT != 0, and gdb is not running, this loop will never
         * exit and weenix will not run.  Make SURE the GDBWAIT is set the way
         * you expect.
         */
        while (gdb_wait) ;
        context_setup(&bootstrap_context, bootstrap, 0, NULL, bstack, PAGE_SIZE, bpdir);
        context_make_active(&bootstrap_context);

        panic("\nReturned to kmain()!!!\n");
}

/**
 * Clears all interrupts and halts, meaning that we will never run
 * again.
 */
static void
hard_shutdown()
{
#ifdef __DRIVERS__
        vt_print_shutdown();
#endif
        __asm__ volatile("cli; hlt");
}

/**
 * This function is called from kmain, however it is not running in a
 * thread context yet. It should create the idle process which will
 * start executing idleproc_run() in a real thread context.  To start
 * executing in the new process's context call context_make_active(),
 * passing in the appropriate context. This function should _NOT_
 * return.
 *
 * Note: Don't forget to set curproc and curthr appropriately.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
bootstrap(int arg1, void *arg2)
{
        /* If the next line is removed/altered in your submission, 20 points will be deducted. */
        /* necessary to finalize page table information */
        pt_template_init();

       /* NOT_YET_IMPLEMENTED("PROCS: bootstrap"); */
    	struct proc *bootproc = proc_create("idle");
    	curproc=bootproc;
    	KASSERT(NULL != curproc);
    	dbg(DBG_PRINT,"(GRADING1A 1.a) KASSERT(NULL != curproc) is success\n");
    	struct kthread *bootthr = kthread_create(bootproc,idleproc_run,0,NULL);
    	curthr=bootthr;
        	KASSERT(PID_IDLE == curproc->p_pid);
        	dbg(DBG_PRINT, "(GRADING1A 1.a) KASSERT(PID_IDLE == curproc->p_pid) is success\n");
    	KASSERT(NULL != curthr);
    	dbg(DBG_PRINT,"(GRADING1A 1.a) KASSERT(NULL != curthr) is success\n");

    	/* dbg(DBG_PRINT,"\n\n @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@   201   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ \n\n"); */
    	/* dbg(DBG_PRINT,"/n/n GRADING1D 1 Sunghan Test"); */


    	context_make_active(&(curthr->kt_ctx));

        panic("weenix returned to bootstrap()!!! BAD!!!\n");
        return NULL;
}

/**
 * Once we're inside of idleproc_run(), we are executing in the context of the
 * first process-- a real context, so we can finally begin running
 * meaningful code.
 *
 * This is the body of process 0. It should initialize all that we didn't
 * already initialize in kmain(), launch the init process (initproc_run),
 * wait for the init process to exit, then halt the machine.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
idleproc_run(int arg1, void *arg2)
{
        int status;
        pid_t child;

        /* create init proc */
        kthread_t *initthr = initproc_create();
        init_call_all();
        GDB_CALL_HOOK(initialized);

        /* Create other kernel threads (in order) */

#ifdef __VFS__
        /* Once you have VFS remember to set the current working directory
         * of the idle and init processes
        NOT_YET_IMPLEMENTED("VFS: idleproc_run");*/

        curproc->p_cwd=vfs_root_vn;
		vref(vfs_root_vn);
		if(initthr!=NULL)
		{
			 dbg(DBG_PRINT,"( GRADING2B ) initthr is not NULL \n");
			initthr->kt_proc->p_cwd=vfs_root_vn;
			vref(vfs_root_vn);
		}

        /* Here you need to make the null, zero, and tty devices using mknod */
        /* You can't do this until you have VFS, check the include/drivers/dev.h
         * file for macros with the device ID's you will need to pass to mknod
        NOT_YET_IMPLEMENTED("VFS: idleproc_run")*/

		 dbg(DBG_PRINT,"( GRADING2B ) CREATING null, zero, and tty devices using mknod \n");

		do_mkdir("/dev");
		do_mknod("/dev/null", S_IFCHR, MKDEVID(1,0));
		do_mknod("/dev/zero", S_IFCHR, MKDEVID(1,1));
		do_mknod("/dev/tty0", S_IFCHR, MKDEVID(2,0));
		do_mknod("/dev/tty1", S_IFCHR, MKDEVID(2,1));
		do_mknod("/dev/sda", S_IFBLK, MKDEVID(1,0));
#endif

        /* Finally, enable interrupts (we want to make sure interrupts
         * are enabled AFTER all drivers are initialized) */
        intr_enable();

        /* Run initproc */
        sched_make_runnable(initthr);
        /* Now wait for it */
        child = do_waitpid(-1, 0, &status);
        KASSERT(PID_INIT == child);

#ifdef __MTP__
        kthread_reapd_shutdown();
#endif


#ifdef __SHADOWD__
        /* wait for shadowd to shutdown */
        shadowd_shutdown();
#endif

#ifdef __VFS__
        /* Shutdown the vfs: */
        dbg_print("weenix: vfs shutdown...\n");
        vput(curproc->p_cwd);
        if (vfs_shutdown())
                panic("vfs shutdown FAILED!!\n");

#endif

        /* Shutdown the pframe system */
#ifdef __S5FS__
        pframe_shutdown();
#endif

        dbg_print("\nweenix: halted cleanly!\n");
        GDB_CALL_HOOK(shutdown);
        hard_shutdown();
        return NULL;
}

/**
 * This function, called by the idle process (within 'idleproc_run'), creates the
 * process commonly refered to as the "init" process, which should have PID 1.
 *
 * The init process should contain a thread which begins execution in
 * initproc_run().
 *
 * @return a pointer to a newly created thread which will execute
 * initproc_run when it begins executing
 */
static kthread_t *
initproc_create(void)
{
      /*  NOT_YET_IMPLEMENTED("PROCS: initproc_create"); */
	proc_t *initproc = proc_create("init");
	KASSERT(NULL != initproc );
	dbg(DBG_PRINT,"(GRADING1A 1.b) KASSERT(NULL != initproc ) is success \n");

	KASSERT(PID_INIT == initproc->p_pid);
	dbg(DBG_PRINT,"(GRADING1A 1.b) KASSERT(PID_INIT == initproc->p_pid) is success\n");
	kthread_t *initthr= kthread_create(initproc,initproc_run,NULL,NULL);
	KASSERT(NULL !=  initthr );
	dbg(DBG_PRINT,"(GRADING1A 1.b) KASSERT(NULL !=  initthr ) is success\n");


	/* dbg(DBG_PRINT,"\n\n @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@   202   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ \n\n"); */
	/* dbg(DBG_PRINT,"/n/n GRADING 1D 1"); */

	return initthr;
	/* return NULL;  */
}

/**
 * The init thread's function changes depending on how far along your Weenix is
 * developed. Before VM/FI, you'll probably just want to have this run whatever
 * tests you've written (possibly in a new process). After VM/FI, you'll just
 * exec "/sbin/init".
 *
 * Both arguments are unused.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
initproc_run(int arg1, void *arg2)
{
	char *argv[] = { NULL };
	char *envp[] = { NULL };
	kernel_execve("/sbin/init", argv, envp);
	return NULL;
}

/*
------------- For Reference ------------------
int kshell_exit(kshell_t *ksh, int argc, char **argv)
{
        panic("kshell: kshell_exit should NEVER be called");
        return 0;
}
*/

int vfs_test_process(kshell_t *ksh, int argc, char **argv)
{
	int p_status;
	KASSERT ( ksh != NULL );
	dbg ( DBG_PRINT, "\n\n GRADING1C 1 Faber Test process starts");
	intr_setipl(IPL_HIGH);
	proc_t *FVTestP = proc_create("VFSTest");
	kthread_t *FVTestKT = kthread_create(FVTestP, (kthread_func_t)vfstest_main, 1, NULL);
	sched_make_runnable(FVTestKT);
	do_waitpid(FVTestP->p_pid, 0, &p_status);
	return 0;
}
int faber_test_process(kshell_t *ksh, int argc, char **argv)
{
	int p_status;

	KASSERT ( ksh != NULL );
	dbg ( DBG_PRINT, "\n\n GRADING1C 1 Faber Test process starts ");

	intr_setipl(IPL_HIGH);
	proc_t *FTestP = proc_create("FaberTest");
	kthread_t *FTestKT = kthread_create(FTestP, faber_thread_test, NULL, NULL);
	sched_make_runnable(FTestKT);
	do_waitpid(FTestP->p_pid, 0, &p_status);

	return 0;
}

int sunghan_test_process(kshell_t *ksh, int argc, char **argv)
{
	int p_status;

	KASSERT ( ksh != NULL );
	dbg ( DBG_PRINT, "\n\n GRADING1D 1 Sunghan Test process starts ");

	intr_setipl(IPL_HIGH);
	proc_t *STestP = proc_create("SunghanTest");
	kthread_t *STestKT = kthread_create(STestP, sunghan_test, NULL, NULL);
	sched_make_runnable(STestKT);
	do_waitpid(STestP->p_pid, 0, &p_status);

	return 0;
}

int sunghanD_test_process(kshell_t *ksh, int argc, char **argv)
{
	int p_status;

	KASSERT ( ksh != NULL );
	dbg ( DBG_PRINT, "\n\n GRADING1D 2 Sunghan Deadlock Test process starts ");

	intr_setipl(IPL_HIGH);
	proc_t *SDTestP = proc_create("FaberTest");
	kthread_t *SDTestKT = kthread_create(SDTestP, sunghan_deadlock_test, NULL, NULL);
	sched_make_runnable(SDTestKT);
	do_waitpid(SDTestP->p_pid, 0, &p_status);

	return 0;
}
