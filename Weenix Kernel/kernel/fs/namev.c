

#include "kernel.h"
#include "globals.h"
#include "types.h"
#include "errno.h"

#include "util/string.h"
#include "util/printf.h"
#include "util/debug.h"

#include "fs/dirent.h"
#include "fs/fcntl.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "fs/vnode.h"

/* This takes a base 'dir', a 'name', its 'len', and a result vnode.
 * Most of the work should be done by the vnode's implementation
 * specific lookup() function, but you may want to special case
 * "." and/or ".." here depnding on your implementation.
 *
 * If dir has no lookup(), return -ENOTDIR.
 *
 * Note: returns with the vnode refcount on *result incremented.
 */
int
lookup(vnode_t *dir, const char *name, size_t len, vnode_t **result)
{
	/*
        NOT_YET_IMPLEMENTED("VFS: lookup");
        return 0;
    */
	int return_status=0;
	KASSERT(NULL != dir);
	dbg(DBG_PRINT,"( GRADING2A 2.a ) KASSERT(NULL != dir) is successful \n");
	KASSERT(NULL != name);
	 dbg(DBG_PRINT,"( GRADING2A 2.a ) KASSERT(NULL != name) is sucessful \n");

	if(!(S_ISDIR(dir->vn_mode)) || (dir->vn_ops->lookup==NULL))
	{
		 dbg(DBG_PRINT,"( GRADING2B ) is not a directory \n");
		return -ENOTDIR;
	}
	if(strcmp(name,".")==0||len == 0)
	{
		dbg(DBG_PRINT,"( GRADING2B ) name (%s) points to current directory\n",name);
		vref(dir);
		*result=dir;
		return 0;
	}
	return_status=dir->vn_ops->lookup(dir,name,len,result);
	KASSERT(NULL != result);
	dbg(DBG_PRINT,"( GRADING2A 2.a ) KASSERT(NULL != result) is sucessful \n");
	return return_status;
}


/* When successful this function returns data in the following "out"-arguments:
 *  o res_vnode: the vnode of the parent directory of "name"
 *  o name: the `basename' (the element of the pathname)
 *  o namelen: the length of the basename
 *
 * For example: dir_namev("/s5fs/bin/ls", &namelen, &name, NULL,
 * &res_vnode) would put 2 in namelen, "ls" in name, and a pointer to the
 * vnode corresponding to "/s5fs/bin" in res_vnode.
 *
 * The "base" argument defines where we start resolving the path from:
 * A base value of NULL means to use the process's current working directory,
 * curproc->p_cwd.  If pathname[0] == '/', ignore base and start with
 * vfs_root_vn.  dir_namev() should call lookup() to take care of resolving each
 * piece of the pathname.
 *
 * Note: A successful call to this causes vnode refcount on *res_vnode to
 * be incremented.
 */
int dir_namev(const char *pathname, size_t *namelen, const char **name,
              vnode_t *base, vnode_t **res_vnode) {


	KASSERT(NULL != pathname);
  dbg(DBG_PRINT, "( GRADING2A 2.b ) KASSERT(NULL != pathname) is successful.\n");

  KASSERT(NULL != namelen);
  dbg(DBG_PRINT, "( GRADING2A 2.b ) KASSERT(NULL != namelen) is successful.\n");

  KASSERT(NULL != name);
  dbg(DBG_PRINT, "( GRADING2A 2.b ) KASSERT(NULL != name) is successful.\n");

  KASSERT(NULL != res_vnode);
  dbg(DBG_PRINT, "( GRADING2A 2.b ) KASSERT(NULL != res_vnode) is successful.\n");
  if (pathname == NULL)
  {
	  dbg(DBG_VFS, "path is NULL.\n");
	  return -ENOENT;
  }

  if (strlen(pathname) == 0)
  {
	  dbg(DBG_VFS, "path is Invalid.\n");
	  return -EINVAL;
  }

  if (strlen(pathname) > MAXPATHLEN)
  {
	  dbg(DBG_VFS, "path Long.\n");
	  return -ENAMETOOLONG;
  }

  vnode_t *lookup_vnode;
  if (base == NULL)
  {
	  base = curproc->p_cwd;
  }

  int i=0,j=0,k=0,lookup_status=0;
  if (pathname[0] == '/')
  {
    base = vfs_root_vn;
    for (; pathname[i] == '/'; ++i);
  }

  vref(base);

  for (;; i += j+k)
  {
    for (j = 0; pathname[i+j] && (pathname[i+j] != '/'); ++j);

    if (j > NAME_LEN)
    {
      vput(base);
      return -ENAMETOOLONG;
    }

    for (k = 0; pathname[i+j+k] == '/'; ++k);

    if (!pathname[i+j+k]) break;

    lookup_status = lookup(base, pathname+i, j, &lookup_vnode);
    vput(base);
    if (lookup_status)
    {
      dbg(DBG_VFS, "lookup returned %d\n", lookup_status);
      return lookup_status;
    }
    base = lookup_vnode;
  }

  if (!S_ISDIR(base->vn_mode))
  {
    dbg(DBG_VFS, "inode %d is not a directory\n", base->vn_vno);
    vput(base);
    return -ENOTDIR;
  }

  if (namelen)
  {
	  *namelen = j;
  }
  if (name)
  {
	  *name = pathname + i;
  }
  KASSERT(NULL != res_vnode);
    dbg(DBG_PRINT, "( GRADING2A 2.b ) KASSERT(NULL != res_vnode) is successful.\n");
  if (res_vnode)
  {
	  *res_vnode = base;
  }
  else
  {
	  vput(base);
  }
  return 0;
}

/* This returns in res_vnode the vnode requested by the other parameters.
 * It makes use of dir_namev and lookup to find the specified vnode (if it
 * exists).  flag is right out of the parameters to open(2); see
 * <weenix/fcntl.h>.  If the O_CREAT flag is specified, and the file does
 * not exist call create() in the parent directory vnode.
 *
 * Note: Increments vnode refcount on *res_vnode.
 */
int
open_namev(const char *pathname, int flag, vnode_t **res_vnode, vnode_t *base)
{
	/*
        NOT_YET_IMPLEMENTED("VFS: open_namev");
        return 0;
    */
	vnode_t *parent_vnode,*lookup_vnode;
	const char *name;
	size_t length;
	int parent_status,current_node_status,return_status;

	parent_status=dir_namev(pathname,&length,&name,base,&parent_vnode);

	/* Path doesn't Exists or Anyother Status*/

	if(parent_status)
	{
		dbg(DBG_PRINT, "( GRADING2B ) dir_namev error: %d\n", parent_status);
		return parent_status;
	}

	current_node_status=lookup(parent_vnode,name,length,&lookup_vnode);
	if((current_node_status != 0) && (current_node_status != -ENOENT))
	{
		dbg(DBG_PRINT, "( GRADING2B ) current_node_status is != 0 and != ENOENT \n");
		vput(parent_vnode);
		return current_node_status;
	}
	if(current_node_status==-ENOENT)
	{
		if(flag & O_CREAT)
		{
			KASSERT(parent_vnode->vn_ops->create != NULL);
			dbg(DBG_PRINT, "( GRADING2A 2.c ) KASSERT(parent_vnode->vn_ops->create != NULL) is successful - conforming dbg() - 1 .\n");
			dbg(DBG_TEST, "( GRADING2A 2.c ) KASSERT(parent_vnode->vn_ops->create != NULL) is successful - conforming dbg() - 2 .\n");
			return_status = parent_vnode->vn_ops->create(parent_vnode,name,length,&lookup_vnode);
			if(return_status < 0)
			{
				dbg(DBG_PRINT, "( GRADING2B ) Create function not successful \n");
				vput(parent_vnode);
				return return_status;
			}
		}
		else
		{
		    dbg(DBG_PRINT, "( GRADING2B inside open_namev )  \n");
			vput(parent_vnode);
			return -ENOENT;
		}
	}
	if(res_vnode)
	{
		dbg(DBG_PRINT, "( GRADING2B ) assigning res_vnode when available  \n");
		*res_vnode=lookup_vnode;
	}
	else
	{
		 dbg(DBG_PRINT, "( GRADING2B ) res_vnode not available  \n");
		vput(lookup_vnode);
	}
	vput(parent_vnode);
	return 0;
}

#ifdef __GETCWD__
/* Finds the name of 'entry' in the directory 'dir'. The name is writen
 * to the given buffer. On success 0 is returned. If 'dir' does not
 * contain 'entry' then -ENOENT is returned. If the given buffer cannot
 * hold the result then it is filled with as many characters as possible
 * and a null terminator, -ERANGE is returned.
 *
 * Files can be uniquely identified within a file system by their
 * inode numbers. */
int
lookup_name(vnode_t *dir, vnode_t *entry, char *buf, size_t size)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_name");
        return -ENOENT;
}


/* Used to find the absolute path of the directory 'dir'. Since
 * directories cannot have more than one link there is always
 * a unique solution. The path is writen to the given buffer.
 * On success 0 is returned. On error this function returns a
 * negative error code. See the man page for getcwd(3) for
 * possible errors. Even if an error code is returned the buffer
 * will be filled with a valid string which has some partial
 * information about the wanted path. */
ssize_t
lookup_dirpath(vnode_t *dir, char *buf, size_t osize)
{
        NOT_YET_IMPLEMENTED("GETCWD: lookup_dirpath");

        return -ENOENT;
}
#endif /* __GETCWD__ */
