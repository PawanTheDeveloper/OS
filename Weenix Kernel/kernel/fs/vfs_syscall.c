
#include "kernel.h"
#include "errno.h"
#include "globals.h"
#include "fs/vfs.h"
#include "fs/file.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/open.h"
#include "fs/fcntl.h"
#include "fs/lseek.h"
#include "mm/kmalloc.h"
#include "util/string.h"
#include "util/printf.h"
#include "fs/stat.h"
#include "util/debug.h"

/* To read a file:
 *      o fget(fd)
 *      o call its virtual read fs_op
 *      o update f_pos
 *      o fput() it
 *      o return the number of bytes read, or an error
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for reading.
 *      o EISDIR
 *        fd refers to a directory.
 *
 * In all cases, be sure you do not leak file refcounts by returning before
 * you fput() a file that you fget()'ed.
 */
int
do_read(int fd, void *buf, size_t nbytes)
{
       /* NOT_YET_IMPLEMENTED("VFS: do_read");
        return -1; */
	dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL READ R)\n");
	file_t *new_file = NULL;
	if(fd < 0 || fd >= NFILES || (curproc->p_files[fd] == NULL))
	{
		dbg(DBG_ERROR, "(GRADING2B VFS_SYSCALL Read R) not a valid file descriptor \n");
		return -EBADF;
	}

	new_file = fget(fd);
	if(new_file == NULL)
	{
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL READ R)\n");
		return -EBADF;
	}


	if(!(new_file->f_mode & FMODE_READ ))
	{
		fput(new_file);
		dbg(DBG_ERROR, "(GRADING2B VFS_SYSCALL READ R) fd is not open for reading\n");
		return -EBADF;
	}

	if(S_ISDIR(new_file->f_vnode->vn_mode))
	{
		fput(new_file);
		dbg(DBG_ERROR, "(GRADING2B VFS_SYSCALL READ R) fd refers to a directory\n");
		return -EISDIR;
	}

	int readsize;
	readsize = new_file->f_vnode->vn_ops->read(new_file->f_vnode,new_file->f_pos,buf,nbytes);
	if(readsize < 0)
	{
			dbg(DBG_ERROR | DBG_PRINT,"GRADING2B ERROR: do_read: Error while the reading the file\n");
			fput(new_file);
			return readsize;
	}
	new_file->f_pos += readsize;
	fput(new_file);
	return readsize;
}

/* Very similar to do_read.  Check f_mode to be sure the file is writable.  If
 * f_mode & FMODE_APPEND, do_lseek() to the end of the file, call the write
 * fs_op, and fput the file.  As always, be mindful of refcount leaks.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for writing.
 */
int
do_write(int fd, const void *buf, size_t nbytes)
{
	/* NOT_YET_IMPLEMENTED("VFS: do_write");
        return -1; */


	file_t *new_file = NULL;
	if(fd < 0 || fd > NFILES || (curproc->p_files[fd] == NULL))
	{
		dbg(DBG_ERROR, "(GRADING2B VFS_SYSCALL Write R) fd is not a valid file descriptor\n");
		return -EBADF;
	}
	new_file = fget(fd);
	if(new_file == NULL)
	{
		dbg(DBG_ERROR, "(GRADING2B VFS_SYSCALL WRITE R) fd is not a valid file descriptor\n");
		return -EBADF;
	}

	if(!(new_file->f_mode & FMODE_APPEND ) && !(new_file->f_mode & FMODE_WRITE ))
	{
		fput(new_file);
		dbg(DBG_ERROR,"(GRADING2B VFS_SYSCALL WRITE R) fd is not open for writing\n");
		return -EBADF;
	}

	int cursor_pos,write_return;


	if(new_file->f_mode & FMODE_APPEND )
	{
		cursor_pos = do_lseek(fd, 0, SEEK_END);
		if(cursor_pos < 0)
		{
			dbg(DBG_ERROR,"GRADING2B ERROR:do_write:  Unable to write file in APPEND mode\n");
			fput(new_file);
			return cursor_pos;
		}

	}

	if(new_file->f_mode & FMODE_WRITE)
	{
		cursor_pos = do_lseek(fd,0,SEEK_CUR);
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL WRITE Mode is Write)\n");
	}
	write_return = new_file->f_vnode->vn_ops->write(new_file->f_vnode,cursor_pos,buf,nbytes);

	KASSERT((S_ISCHR(new_file->f_vnode->vn_mode)) ||(S_ISBLK(new_file->f_vnode->vn_mode)) ||((S_ISREG(new_file->f_vnode->vn_mode)) && (new_file->f_pos <= new_file->f_vnode->vn_len)));
    dbg(DBG_PRINT,"(GRADING2A 3.a) Write is successful\n");
	new_file->f_pos += write_return;
	fput(new_file);
	return write_return;
}

/*
 * Zero curproc->p_files[fd], and fput() the file. Return 0 on success
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't a valid open file descriptor.
 */
int
do_close(int fd)
{
	/*
        NOT_YET_IMPLEMENTED("VFS: do_close");
        return -1;
    	*/
	if(fd < 0 || fd >= NFILES || curproc->p_files[fd] == NULL)
	{
		dbg(DBG_ERROR, "(GRADING2B VFS_SYSCALL CLOSE R) fd isn't a valid open file descriptor \n");
		return -EBADF;
	}

	if ( curproc->p_files[fd] == NULL )
	{
		dbg(DBG_ERROR, "(GRADING2B VFS_SYSCALL CLOSE R) fd is null \n");
		return -EBADF;
	}

	fput( curproc->p_files[fd] );
	curproc->p_files[fd] = NULL;
	dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL CLOSE  R)\n");
	return 0;

}

/* To dup a file:
 *      o fget(fd) to up fd's refcount
 *      o get_empty_fd()
 *      o point the new fd to the same file_t* as the given fd
 *      o return the new file descriptor
 *
 * Don't fput() the fd unless something goes wrong.  Since we are creating
 * another reference to the file_t*, we want to up the refcount.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't an open file descriptor.
 *      o EMFILE
 *        The process already has the maximum number of file descriptors open
 *        and tried to open a new one.
 */
int
do_dup(int fd)
{
	/*
	NOT_YET_IMPLEMENTED("VFS: do_dup");
        return -1;
     	*/
	int new_filedes;
	if(fd < 0 || fd >= NFILES || (curproc->p_files[fd] == NULL))
	{
		dbg(DBG_ERROR, "(GRADING2B VFS_SYSCALL DUP2 R) fd isn't an open file descriptor\n");
		return -EBADF;
	}

	file_t *newfile = fget(fd);
	new_filedes = get_empty_fd(curproc);
	if( new_filedes == -EMFILE )
	{
		dbg(DBG_ERROR | DBG_PRINT,"GRADING2B ERROR: do_dup: already has the maximum number of file descriptors open \n");
		fput(newfile);
		return -EMFILE;
	}

	curproc->p_files[new_filedes] = newfile;
	dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL DUP R) returning new fd \n");
	return new_filedes;
}

/* Same as do_dup, but insted of using get_empty_fd() to get the new fd,
 * they give it to us in 'nfd'.  If nfd is in use (and not the same as ofd)
 * do_close() it first.  Then return the new file descriptor.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        ofd isn't an open file descriptor, or nfd is out of the allowed
 *        range for file descriptors.
 */
int
do_dup2(int ofd, int nfd)
{
	/* NOT_YET_IMPLEMENTED("VFS: do_dup2");
        	return -1;
       */

	/* checking OFD */
	file_t *newfile = NULL;
	if(ofd < 0 || ofd >= NFILES || (curproc->p_files[ofd] == NULL))
	{
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL DUP2 R) ofd isn't an open file descriptor\n");
		return -EBADF;
	}

	if(nfd < 0 || nfd >= NFILES)
	{
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL DUP2 R) nfd is out of the allowed range for file descriptors\n");
		return -EBADF;
	}

	newfile = fget(ofd);
	if(ofd == nfd)
	{
		fput(newfile);
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL DUP2 R)\n");
		return nfd;
	}

	if(curproc->p_files[nfd] != NULL)
	{
		int status = do_close(nfd);
		if(status < 0)
		{
			dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL DUP2 IF) close status < 0 \n");
			return status;
		}
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL DUP2)\n");
	}
	curproc->p_files[nfd] = newfile;
	dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL DUP2 Returning successful  R)\n");
	return nfd;

}

/*
 * This routine creates a special file of the type specified by 'mode' at
 * the location specified by 'path'. 'mode' should be one of S_IFCHR or
 * S_IFBLK (you might note that mknod(2) normally allows one to create
 * regular files as well-- for simplicity this is not the case in Weenix).
 * 'devid', as you might expect, is the device identifier of the device
 * that the new special file should represent.
 *
 * You might use a combination of dir_namev, lookup, and the fs-specific
 * mknod (that is, the containing directory's 'mknod' vnode operation).
 * Return the result of the fs-specific mknod, or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        mode requested creation of something other than a device special
 *        file.
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_mknod(const char *path, int mode, unsigned devid)
{
	/*
        NOT_YET_IMPLEMENTED("VFS: do_mknod");
        return -1;
    	*/
	vnode_t *random,*final_res;
	const char *name;
	size_t length_name;

	if( strlen(path) > MAXPATHLEN )
	{
		return -ENAMETOOLONG;
	}

	int dirname_return, return_lookup;
	if( ( mode != S_IFCHR && mode != S_IFBLK ) || path == NULL)
	{
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL MKNOD  R) Invalid Path \n");
		return -EINVAL;
	}
	dirname_return = dir_namev(path, &length_name, &name, NULL, &final_res);
	if(dirname_return < 0)
	{
		dbg(DBG_ERROR | DBG_PRINT,"GRADING2B ERROR: do_mknod:Dir_NamevUnable to resolve the file path\n");
		return dirname_return;
	}

	return_lookup = lookup(final_res, name, length_name, &random);
	if(return_lookup == 0)
	{
		dbg(DBG_ERROR | DBG_PRINT,"GRADING2B ERROR: do_mknod:Lookup Unable to resolve the file path\n");
		vput(final_res);
		vput(random);
		return -EEXIST;
	}

	int result;
	result = final_res->vn_ops->mknod(final_res, name, length_name, mode, devid);
	KASSERT(NULL !=  final_res->vn_ops->mknod);
	dbg(DBG_PRINT,"(GRADING2A 3.b) KASSERT\n");
	vput(final_res);
	dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL MKNOD Returning Successfully  R)\n");
	return result;

}

/* Use dir_namev() to find the vnode of the dir we want to make the new
 * directory in.  Then use lookup() to make sure it doesn't already exist.
 * Finally call the dir's mkdir vn_ops. Return what it returns.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        path already exists.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_mkdir(const char *path)
{
	/*
        NOT_YET_IMPLEMENTED("VFS: do_mkdir");
        return -1;
    	*/
	if(strlen(path) > MAXPATHLEN )
	{
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL MKDIR R) path was too long \n");
		return -ENAMETOOLONG;
	}
	vnode_t *random,*final_res;
	const char *name;
	size_t length_name;

	int dirname_return, return_lookup;
	dirname_return = dir_namev(path, &length_name, &name, NULL, &final_res);
	if(dirname_return < 0)
	{
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL MKDIR R)\n");
		return dirname_return;
	}

	if(strlen(name) > NAME_LEN)
	{
    		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL MKDIR R) name was too long\n");
    		vput(final_res);
		return -ENAMETOOLONG;
	}

	return_lookup = lookup(final_res, name, length_name, &random);
	if(return_lookup == 0)
	{
		vput(final_res);
		vput(random);
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL MKDIR R) path already exists.\n");
		return -EEXIST;
	}
	if( ! S_ISDIR(final_res->vn_mode) )
	{
		vput(final_res);
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL MKDIR R) is not a directory \n");
		return -ENOTDIR;
	}
	int result;

	KASSERT(NULL != final_res->vn_ops->mkdir);
	dbg(DBG_PRINT,"(GRADING2A 3.c) KASSERT is successful\n");
	result = final_res->vn_ops->mkdir(final_res, name, length_name);
	dbg(DBG_PRINT,"(GRADING2A 3.c)\n");
	vput(final_res);
	dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL MKDIR: Successfully created. R)\n");
	return result;
}

/* Use dir_namev() to find the vnode of the directory containing the dir to be
 * removed. Then call the containing dir's rmdir v_op.  The rmdir v_op will
 * return an error if the dir to be removed does not exist or is not empty, so
 * you don't need to worry about that here. Return the value of the v_op,
 * or an error.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EINVAL
 *        path has "." as its final component.
 *      o ENOTEMPTY
 *        path has ".." as its final component.
 *      o ENOENT
 *        A directory component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_rmdir(const char *path)
{
    	/*
	NOT_YET_IMPLEMENTED("VFS: do_rmdir");
        return -1;
    	*/

	if(strlen(path) > MAXPATHLEN )
	{
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL RMDIR R) path was too long \n");
		return -ENAMETOOLONG;
	}

	vnode_t *random, *final_res;
	const char *name;
	size_t length_name;

	int dirname_return, return_lookup;
	dirname_return = dir_namev(path, &length_name, &name, NULL, &final_res);
	if( dirname_return )
	{
	    	dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL RMDIR R)\n");
		return dirname_return;
	}
	if( strcmp(name, "..") == 0 )
	{
		vput(final_res);
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL RMDIR R) path has .. as its final component \n");
		return -ENOTEMPTY;
	}
	if(strcmp(name,".") == 0)
	{
		vput(final_res);
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL RMDIR R) path has . as its final component\n");
		return -EINVAL;
	}
	if(!S_ISDIR(final_res->vn_mode))
	{
		vput(final_res);
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL RMDIR R) is not a directory\n");
		return -ENOTDIR;
	}
	int result;
	result = final_res->vn_ops->rmdir(final_res, name, length_name);
	KASSERT(NULL != final_res->vn_ops->rmdir);
	dbg(DBG_PRINT,"(GRADING2A 3.d) KASSERT is successful conforming dbg() - 1 \n");
	dbg(DBG_TEST,"(GRADING2A 3.d) KASSERT is successful conforming dbg() - 2 \n");
	vput(final_res);
	dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL RMDIR R)\n");
	return result;
}

/*
 * Same as do_rmdir, but for files.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EISDIR
 *        path refers to a directory.
 *      o ENOENT
 *        A component in path does not exist.
 *      o ENOTDIR
 *        A component used as a directory in path is not, in fact, a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_unlink(const char *path)
{
	/*
	NOT_YET_IMPLEMENTED("VFS: do_unlink");
        return -1;
        */
	if(strlen(path) > MAXPATHLEN )
	{
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL UNLINK R) path was too long \n");
		return -ENAMETOOLONG;
	}

	vnode_t *random, *final_res;
	const char *name;
	size_t length_name;

	int dirname_return, return_lookup;
	dirname_return = dir_namev(path, &length_name, &name, NULL, &final_res);
	if(dirname_return)
	{
	    	dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL UNLINK R)\n");
	    	return dirname_return;
	}

	if( ! strcmp(name, ".") )
	{
		vput(final_res);
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL UNLINK R)\n");
		return -EINVAL;
	}


	if( ! strcmp(name, "..") )
	{
		vput(final_res);
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL UNLINK R)\n");
		return -ENOTEMPTY;
	}

	if( ! S_ISDIR(final_res->vn_mode) )
	{
		vput(final_res);
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL UNLINK R) is not a directory \n");
		return -ENOTDIR;
	}
	return_lookup = lookup(final_res, name, length_name, &random);
	if(return_lookup )
	{
		vput(final_res);
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL UNLINK R)\n");
		return return_lookup;
	}

	if(S_ISDIR(random->vn_mode))
	{
		vput(random);
		vput(final_res);
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL UNLINK R) path refers to a directory\n");
		return -EISDIR;
	}
	int result;
	result = final_res->vn_ops->unlink(final_res, name, length_name);
	KASSERT(NULL != final_res->vn_ops->unlink);
	dbg(DBG_PRINT,"(GRADING2A 3.e) KASSERT is successful conforming dbg() - 1 \n");
	dbg(DBG_TEST,"(GRADING2A 3.e) KASSERT is successful conforming dbg() - 2 \n");
	vput(final_res);
	vput(random);
	dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL UNLINK Success R)\n");
	return result;
}

/* To link:
 *      o open_namev(from)
 *      o dir_namev(to)
 *      o call the destination dir's (to) link vn_ops.
 *      o return the result of link, or an error
 *
 * Remember to vput the vnodes returned from open_namev and dir_namev.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EEXIST
 *        to already exists.
 *      o ENOENT
 *        A directory component in from or to does not exist.
 *      o ENOTDIR
 *        A component used as a directory in from or to is not, in fact, a
 *        directory.
 *      o ENAMETOOLONG
 *        A component of from or to was too long.
 *      o EISDIR
 *        from is a directory.
 */
int
do_link(const char *from, const char *to)
{
        /* NOT_YET_IMPLEMENTED("VFS: do_link");
        return -1; */

	if( strlen(to) > NAME_LEN || strlen(from) > NAME_LEN )
	{
		dbg(DBG_PRINT, "(GRADING3D VFS_SYSCALL DOLINK R) path was too long \n");
		return -ENAMETOOLONG;
	}

	vnode_t *random, *final_res, *open_res;
	const char *name;
	size_t length_name;
	int op = 0;
	int dirname_return, return_lookup;

	open_namev(from, op, &open_res, NULL);
	if( S_ISDIR(open_res->vn_mode) )
	{
		vput(open_res);
		dbg(DBG_PRINT, "(GRADING3D VFS_SYSCALL DOLINK R) is a directory \n");
		return -EISDIR;
	}

	dirname_return = dir_namev(to, &length_name, &name, NULL, &final_res);
	if(dirname_return)
	{
	    	dbg(DBG_PRINT, "(GRADING3D VFS_SYSCALL DOLINK dirname R)\n");
	    	return dirname_return;
	}

	int result;
	return_lookup = lookup(final_res, name, length_name, &random);
	if(return_lookup != 0)
	{
		result = final_res->vn_ops->link(open_res, final_res, name, length_name);
		KASSERT(NULL != final_res->vn_ops->link);
		vput(open_res);
		vput(final_res);
		dbg(DBG_PRINT, "(GRADING3D VFS_SYSCALL DOLINK lookup not 0)\n");
		return result;
	}
	else
	{
		dbg(DBG_PRINT, "(GRADING3D VFS_SYSCALL DOLINK lookup is 0)\n");
		vput(open_res);
		vput(final_res);
		vput(random);
		return -EEXIST;
	}
}

/*      o link newname to oldname
 *      o unlink oldname
 *      o return the value of unlink, or an error
 *
 * Note that this does not provide the same behavior as the
 * Linux system call (if unlink fails then two links to the
 * file could exist).
 */
int
do_rename(const char *oldname, const char *newname)
{
        NOT_YET_IMPLEMENTED("VFS: do_rename");
        return -1;
}

/* Make the named directory the current process's cwd (current working
 * directory).  Don't forget to down the refcount to the old cwd (vput()) and
 * up the refcount to the new cwd (open_namev() or vget()). Return 0 on
 * success.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        path does not exist.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 *      o ENOTDIR
 *        A component of path is not a directory.
 */
int
do_chdir(const char *path)
{
	/*
	NOT_YET_IMPLEMENTED("VFS: do_chdir");
        return -1;
    	*/

	int openname_return;
	vnode_t *final_res;
	openname_return = open_namev(path, NULL, &final_res, NULL);
	if(openname_return < 0)
	{
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL CHANGEDIR  R)\n");
		return openname_return;
	}

	if(!S_ISDIR(final_res -> vn_mode))
	{
		vput(final_res);
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL CHANGEDIR  R) path is not a directory \n");
		return -ENOTDIR;
	}

	vput(curproc->p_cwd);
	curproc->p_cwd = final_res;
	dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL CHANGEDIR Successful  R)\n");
	return 0;
}

/* Call the readdir fs_op on the given fd, filling in the given dirent_t*.
 * If the readdir fs_op is successful, it will return a positive value which
 * is the number of bytes copied to the dirent_t.  You need to increment the
 * file_t's f_pos by this amount.  As always, be aware of refcounts, check
 * the return value of the fget and the virtual function, and be sure the
 * virtual function exists (is not null) before calling it.
 *
 * Return either 0 or sizeof(dirent_t), or -errno.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        Invalid file descriptor fd.
 *      o ENOTDIR
 *        File descriptor does not refer to a directory.
 */
int
do_getdent(int fd, struct dirent *dirp)
{
	KASSERT(curproc!=NULL);
	KASSERT(dirp!=NULL);
	if( fd < 0 || fd >= NFILES || (curproc->p_files[fd] == NULL))
	{
		dbg(DBG_ERROR | DBG_PRINT,"GRADING2B ERROR: do_getdent: Invalid file descriptor fd\n");
			return -EBADF;
	}
	file_t *open_file = fget(fd);

	KASSERT(open_file != NULL);

	if(!S_ISDIR(open_file->f_vnode->vn_mode))
	{
		dbg(DBG_ERROR | DBG_PRINT,"GRADING2B ERROR: do_getdent: File descriptor doesn't refer to a directory\n");
		fput(open_file);
		return -ENOTDIR;
	}
	KASSERT(open_file->f_vnode->vn_ops->readdir);
	int i = 0;
	i = (open_file->f_vnode->vn_ops->readdir)(open_file->f_vnode, open_file->f_pos, dirp);
	open_file->f_pos = open_file->f_pos + i;
	if(i <= 0)
	{
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL GETDENT ) open_file_status < 0 \n");
		fput(open_file);
		return i;
	}
	else if(i == 0)
	{
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL GETDENT ) open_file_status == 0 \n");
		fput(open_file);
		return 0;
	}
	else
	{
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL GETDENT ) open_file_status > 0 \n");
		fput(open_file);
		return sizeof(dirent_t);
	}
	/*NOT_YET_IMPLEMENTED("VFS: do_getdent");*/
	dbg(DBG_PRINT,"GRADING2B INFO: Successfully performed getdent operation on file\n");
}

/*
 * Modify f_pos according to offset and whence.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not an open file descriptor.
 *      o EINVAL
 *        whence is not one of SEEK_SET, SEEK_CUR, SEEK_END; or the resulting
 *        file offset would be negative.
 */
int
do_lseek(int fd, int offset, int whence)
{
        /* NOT_YET_IMPLEMENTED("VFS: do_lseek");
        return -1;
        */
	file_t *newfile;
	if( fd < 0 || fd >= NFILES || (curproc->p_files[fd] == NULL))
	{
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL DUP2 R) fd is not an open file descriptor \n");
		return -EBADF;
	}

	if(whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
	{
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL LSEEK R) whence is not one of SEEK_SET, SEEK_CUR, SEEK_END \n");
		return -EINVAL;
	}
	newfile = fget(fd);
	int position_old;
	position_old = newfile->f_pos;

	if(whence == SEEK_CUR)
	{

		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL LSEEK IF) SEEK_CUR \n");

		if( (position_old + offset) < 0 )
		{
			fput(newfile);
			dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL LSEEK R) pos + offset < 0 \n");
			return -EINVAL;
		}
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL LSEEK IF) SEEK_CUR complete \n");
		newfile->f_pos += offset;
	}


	if(whence == SEEK_SET)
	{
		if(offset < 0)
		{
			fput(newfile);
			dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL LSEEK R) offset < 0 \n");
			return -EINVAL;
		}
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL LSEEK IF) SEEK_SET complete \n");
		newfile->f_pos = offset;
	}

	if(whence == SEEK_END)
	{
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL LSEEK IF) SEEK_END \n");
		if((newfile->f_vnode->vn_len + offset) < 0)
		{
			fput(newfile);
			dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL LSEEK R) negative offset \n");
			return -EINVAL;
		}
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL LSEEK IF) SEEK_END complete \n");
		newfile->f_pos = newfile->f_vnode->vn_len + offset;
	}

	fput(newfile);
	dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL LSEEK R) LSEEK completed \n");
	return newfile->f_pos;
}

/*
 * Find the vnode associated with the path, and call the stat() vnode operation.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o ENOENT
 *        A component of path does not exist.
 *      o ENOTDIR
 *        A component of the path prefix of path is not a directory.
 *      o ENAMETOOLONG
 *        A component of path was too long.
 */
int
do_stat(const char *path, struct stat *buf)
{
	/*
        NOT_YET_IMPLEMENTED("VFS: do_stat");
        return -1;
    	*/

	int ret_stat,openname_return;
	vnode_t *final_res;
	size_t len_name;

	if(strlen(path) > MAXPATHLEN )
	{
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL STAT R) path was too long \n");
		return -ENAMETOOLONG;
	}

	openname_return = open_namev(path,NULL,&final_res,NULL);
	if(openname_return)
	{
		dbg(DBG_PRINT, "(GRADING2B VFS_SYSCAL STAT IR)\n");
		return openname_return;
	}
	ret_stat = final_res->vn_ops->stat(final_res,buf);
	KASSERT(final_res->vn_ops->stat);
	dbg(DBG_PRINT,"(GRADING2A 3.f) KASSERT is successful conforming dbg() - 1 \n");
	dbg(DBG_TEST,"(GRADING2A 3.f) KASSERT is successful conforming dbg() - 2 \n");
	vput(final_res);
	dbg(DBG_PRINT, "(GRADING2B VFS_SYSCALL STAT R)\n");
	return ret_stat;
}

#ifdef __MOUNTING__
/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutely sure your Weenix is perfect.
 *
 * This is the syscall entry point into vfs for mounting. You will need to
 * create the fs_t struct and populate its fs_dev and fs_type fields before
 * calling vfs's mountfunc(). mountfunc() will use the fields you populated
 * in order to determine which underlying filesystem's mount function should
 * be run, then it will finish setting up the fs_t struct. At this point you
 * have a fully functioning file system, however it is not mounted on the
 * virtual file system, you will need to call vfs_mount to do this.
 *
 * There are lots of things which can go wrong here. Make sure you have good
 * error handling. Remember the fs_dev and fs_type buffers have limited size
 * so you should not write arbitrary length strings to them.
 */
int
do_mount(const char *source, const char *target, const char *type)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_mount");
        return -EINVAL;
}

/*
 * Implementing this function is not required and strongly discouraged unless
 * you are absolutley sure your Weenix is perfect.
 *
 * This function delegates all of the real work to vfs_umount. You should not worry
 * about freeing the fs_t struct here, that is done in vfs_umount. All this function
 * does is figure out which file system to pass to vfs_umount and do good error
 * checking.
 */
int
do_umount(const char *target)
{
        NOT_YET_IMPLEMENTED("MOUNTING: do_umount");
        return -EINVAL;
}
#endif
