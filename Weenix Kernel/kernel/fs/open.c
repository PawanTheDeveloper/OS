

#include "globals.h"
#include "errno.h"
#include "fs/fcntl.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/stat.h"
#include "util/debug.h"

/* find empty index in p->p_files[] */
int
get_empty_fd(proc_t *p)
{
        int fd;

        for (fd = 0; fd < NFILES; fd++) {
                if (!p->p_files[fd])
                        return fd;
        }

        dbg(DBG_ERROR | DBG_PRINT, "GRADING2B ERROR: get_empty_fd: out of file descriptors "
            "for pid %d\n", curproc->p_pid);
        return -EMFILE;
}

/*
 * There a number of steps to opening a file:
 *      1. Get the next empty file descriptor.
 *      2. Call fget to get a fresh file_t.
 *      3. Save the file_t in curproc's file descriptor table.
 *      4. Set file_t->f_mode to OR of FMODE_(READ|WRITE|APPEND) based on
 *         oflags, which can be O_RDONLY, O_WRONLY or O_RDWR, possibly OR'd with
 *         O_APPEND or O_CREAT.
 *      5. Use open_namev() to get the vnode for the file_t.
 *      6. Fill in the fields of the file_t.
 *      7. Return new fd.
 *
 * If anything goes wrong at any point (specifically if the call to open_namev
 * fails), be sure to remove the fd from curproc, fput the file_t and return an
 * error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        oflags is not valid.
 *      o EMFILE
 *        The process already has the maximum number of files open.
 *      o ENOMEM
 *        Insufficient kernel memory was available.
 *      o ENAMETOOLONG
 *        A component of filename was too long.
 *      o ENOENT
 *        O_CREAT is not set and the named file does not exist.  Or, a
 *        directory component in pathname does not exist.
 *      o EISDIR
 *        pathname refers to a directory and the access requested involved
 *        writing (that is, O_WRONLY or O_RDWR is set).
 *      o ENXIO
 *        pathname refers to a device special file and no corresponding device
 *        exists.
 */
int
do_open(const char *filename, int oflags)
{
        /*
        NOT_YET_IMPLEMENTED("VFS: do_open");
        return -1;
        */

        int fd =0;

        fd=get_empty_fd(curproc);
        dbg(DBG_PRINT, "FD: %d, oflags=%d",fd,oflags);
        if(fd == -EMFILE)
        {
        	dbg(DBG_ERROR | DBG_PRINT, "GRADING2B ERROR: get_empty_fd: maximum number of files open ");
        	return -EMFILE;
        }
        file_t *newfile = fget(-1);
        if(newfile==NULL)
        {
        	dbg(DBG_ERROR, "(GRADING2B OPEN ) Insufficient kernel memory \n");
        	fput(newfile);
        	return -ENOMEM;
        }
        if(strlen(filename)>NAME_LEN)
        {
        	dbg(DBG_ERROR, "(GRADING2B OPEN ) : Filename is too long. \n");
        	fput(newfile);
        	return -ENAMETOOLONG;
        }

        if((oflags == O_RDONLY ) || (oflags == (O_RDONLY|O_CREAT)) || (oflags == (O_RDONLY|O_TRUNC)))
		{
			newfile->f_mode = FMODE_READ;
			dbg(DBG_PRINT, "(GRADING2B OPEN ) : Fmode Read \n");
		}
        else if((oflags == O_WRONLY) || (oflags ==(O_WRONLY|O_CREAT)) || (oflags == (O_WRONLY|O_TRUNC)))
		{
			newfile->f_mode = FMODE_WRITE;
			dbg(DBG_PRINT, "(GRADING2B OPEN ) : Fmode Write \n");
		}
        else if(oflags == (O_RDWR | O_APPEND))
		{
			newfile->f_mode = FMODE_WRITE | FMODE_READ | FMODE_APPEND;
			dbg(DBG_PRINT, "(GRADING2B OPEN ) Fmode Read, Write, Append \n");
		}

        else if(oflags == (O_RDONLY | O_APPEND))
		{
			newfile->f_mode = FMODE_READ | FMODE_APPEND;
			dbg(DBG_PRINT, "(GRADING2B OPEN ) Fmode Read, Append \n");
		}

        else if(oflags == (O_WRONLY | O_APPEND))
		{
			newfile->f_mode = FMODE_WRITE | FMODE_APPEND;
			dbg(DBG_PRINT, "(GRADING2B OPEN ) Fmode Write, Append \n");
		}

        else if((oflags == O_RDWR) || (oflags == (O_RDWR|O_CREAT)) || (oflags == (O_RDWR|O_TRUNC)))
		{
			newfile->f_mode = FMODE_READ|FMODE_WRITE;
			dbg(DBG_PRINT, "(GRADING2B OPEN ) Fmode Read, Write \n");
		}

		else
		{
			dbg(DBG_ERROR, "(GRADING2B OPEN R) Invalid oflags \n");
			return -EINVAL;
		}


		vnode_t *final_res;
		vnode_t *base = NULL;
		int openname_return;

		/*not implemented ENXIO and ENOMEM and EMFILE */
		openname_return=open_namev(filename,oflags,&final_res, base);
        if( final_res == NULL || openname_return != 0 )
        {

        	dbg(DBG_PRINT, "(GRADING2B OPEN : File doesn't exist)\n");
        	do_close(fd);
        	curproc->p_files[fd] = NULL;
			return openname_return;
        }
        newfile->f_vnode = final_res;

        if((((oflags & 3) == O_WRONLY) || ((oflags & 3 ) == O_RDWR)) && (S_ISDIR(newfile->f_vnode->vn_mode)))
        {
        	do_close(fd);
			vput(final_res);
			dbg(DBG_ERROR, "(GRADING2B OPEN R) Is Directory \n");
			return -EISDIR;
        }
        curproc->p_files[fd] = newfile;
       /* newfile->f_pos = 0;*/
        dbg(DBG_PRINT, "(GRADING2B OPEN R) returning File Descriptor \n");
        return fd;
}
