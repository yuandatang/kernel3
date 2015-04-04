/******************************************************************************/
/* Important Spring 2015 CSCI 402 usage information:                          */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

/*
 *  FILE: vfs_syscall.c
 *  AUTH: mcc | jal
 *  DESC:
 *  DATE: Wed Apr  8 02:46:19 1998
 *  $Id: vfs_syscall.c,v 1.10 2014/12/22 16:15:17 william Exp $
 */

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
int do_read(int fd, void *buf, size_t nbytes) {
    /* NOT_YET_IMPLEMENTED("VFS: do_read");
     return -1;*/
	file_t *file = NULL;
	file = fget(fd);
	if(NULL == file || !(file->f_mode & FMODE_READ)){
		if(file) fput(file);
		return -EBADF;
	}

	if(S_ISDIR(file->f_vnode->vn_mode)) {
		fput(file);
		return -EISDIR;
	}

	int bytesread = file->f_vnode->vn_ops->read(file->f_vnode, file->f_pos, buf, nbytes);
	file->f_pos = file->f_pos + bytesread;
	fput(file);
	return bytesread;
}

/* Very similar to do_read.  Check f_mode to be sure the file is writable.  If
 * f_mode & FMODE_APPEND, do_lseek() to the end of the file, call the write
 * fs_op, and fput the file.  As always, be mindful of refcount leaks.
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd is not a valid file descriptor or is not open for writing.
 */
int do_write(int fd, const void *buf, size_t nbytes) {
	/*NOT_YET_IMPLEMENTED("VFS: do_write");
	return -1;*/
	file_t *file = fget(fd);
	if(NULL == file || !(file->f_mode & FMODE_WRITE)){
		return -EBADF;
	}
	int seek_pos = file->f_pos;
	if(file->f_mode & FMODE_APPEND)
		seek_pos = do_lseek(fd, 0, SEEK_END);

	int byteswrote = file->f_vnode->vn_ops->write(file->f_vnode, seek_pos, buf, nbytes);
	fput(file);
	return byteswrote;
}

/*
 * Zero curproc->p_files[fd], and fput() the file. Return 0 on success
 *
 * Error cases you must handle for this function at the VFS level:
 *      o EBADF
 *        fd isn't a valid open file descriptor.
 */
int do_close(int fd) {
    /*NOT_YET_IMPLEMENTED("VFS: do_close");
    return -1;*/
	if(fd < 0 || fd > NFILES || NULL == curproc->p_files[fd])
		return -EBADF;

	file_t *file = curproc->p_files[fd];
	curproc->p_files[fd] = NULL;
	fput(file);
	/*KASSERT(curproc->p_files[fd] == NULL);*/
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
int do_dup(int fd) {
    /*NOT_YET_IMPLEMENTED("VFS: do_dup");
    return -1;*/
	file_t *new_handle = fget(fd);
	if(NULL == new_handle){
		return -EBADF;
	}
	int new_fd = get_empty_fd(curproc);
	if (new_fd == -EMFILE) {
		fput(new_handle);
		return -EMFILE;
	}
	curproc->p_files[new_fd] = new_handle;
	return new_fd;
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
int do_dup2(int ofd, int nfd) {
    /*NOT_YET_IMPLEMENTED("VFS: do_dup2");
    return -1;*/
	file_t *file = fget(ofd);
	if(NULL == file || nfd < 0 || nfd > NFILES){
		if(file) fput(file);
		return -EBADF;
	}
	if(curproc->p_files[nfd] == curproc->p_files[ofd]){
		if(ofd != nfd) fput(file);
		return nfd;
	}

	if (curproc->p_files[nfd] != NULL)
		do_close(nfd);

	curproc->p_files[nfd] = file;
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
int do_mknod(const char *path, int mode, unsigned devid) {
    /* NOT_YET_IMPLEMENTED("VFS: do_mknod");
     return -1;*/
	if(!S_ISCHR(mode) || !S_ISBLK(mode))
		return -EINVAL;

	vnode_t *dir_vnode = NULL; /* vnode of the parent */
	size_t filename_len = 0 ;
	const char *filename;
	int dir_namev_retval = dir_namev(path, &filename_len, &filename, NULL, &dir_vnode);
	if(dir_namev_retval < 0) {
		return dir_namev_retval; /* ENOENT, ENOTDIR, ENAMETOOLONG*/
	}
	else {
		vnode_t *file_vnode = NULL;
		int lookup_retval = lookup(dir_vnode, filename, filename_len, &file_vnode);
		if(lookup_retval == 0) {
			vput(dir_vnode);
			return -EEXIST;
		}
		else {
			vput(file_vnode);
			KASSERT(NULL != dir_vnode->vn_ops->mknod);
			return dir_vnode->vn_ops->mknod(dir_vnode, filename, filename_len, mode, devid);
		}
	}
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
int do_mkdir(const char *path) {
	/*
	NOT_YET_IMPLEMENTED("VFS: do_mkdir");
	return -1;
	*/

	vnode_t *dir_vnode = NULL; /* vnode of the parent */
	size_t filename_len = 0 ;
	const char *filename;
	int dir_namev_retval = dir_namev(path, &filename_len, &filename, NULL, &dir_vnode);
	if(dir_namev_retval < 0) {
		return dir_namev_retval; /* ENOENT, ENOTDIR, ENAMETOOLONG*/
	}
	else {
		vnode_t *file_vnode = NULL;
		int lookup_retval = lookup(dir_vnode, filename, filename_len, &file_vnode);
		if(lookup_retval == 0) {
			vput(dir_vnode);
			return -EEXIST;
		}
		else {
			vput(file_vnode);
			KASSERT(NULL != dir_vnode->vn_ops->mkdir);
			return dir_vnode->vn_ops->mkdir(dir_vnode, filename, filename_len);
		}
	}
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
int do_rmdir(const char *path) {
	/*
	NOT_YET_IMPLEMENTED("VFS: do_rmdir");
	return -1;
	*/
	vnode_t *temp = NULL;
	size_t temp_len = 0;
	const char *temp_name;
	int dir_namev_retval = dir_namev(path, &temp_len, &temp_name, NULL, &temp);
	if(dir_namev_retval < 0) {
		/*vput(temp); vref() not called when it's a failure */
		return dir_namev_retval; /* ENOENT,ENOTDIR, ENAMETOOLONG */
	}else {
		if (strcmp(temp_name,".")) {
			vput(temp);
			return -EINVAL;
		}
		else if (strcmp(temp_name,"..")) {
			vput(temp);
			return -ENOTEMPTY;
		}
		KASSERT(NULL != temp->vn_ops->rmdir);
		return temp->vn_ops->rmdir(temp, temp_name, temp_len);
	}
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
int do_unlink(const char *path) {
	/*NOT_YET_IMPLEMENTED("VFS: do_unlink");
	return -1;*/
	vnode_t *dir_vnode = NULL;
	size_t filename_len = 0;
	const char *filename;
	int dir_namev_retval = dir_namev(path, &filename_len, &filename, NULL, &dir_vnode);
	if(dir_namev_retval < 0) {
		vput(dir_vnode);
		return dir_namev_retval; /* ENOENT,ENOTDIR, ENAMETOOLONG */
	}else {
		vnode_t *file_vnode = NULL;
		int lookup_retval = lookup(dir_vnode, filename, filename_len, &file_vnode);
		if(lookup_retval < 0) {
			vput(dir_vnode);
			return lookup_retval;
		}
		if(S_ISDIR(file_vnode->vn_mode)) {
			vput(file_vnode);
			vput(dir_vnode);
			return -EISDIR;
		}
		vput(file_vnode);
		KASSERT(NULL != dir_vnode->vn_ops->unlink);
		return dir_vnode->vn_ops->unlink(dir_vnode, filename, filename_len);
	}
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
int do_link(const char *from, const char *to) {
	/*NOT_YET_IMPLEMENTED("VFS: do_link");
	return -1;
	*/
	vnode_t* from_file_vnode;
	int resp = open_namev(from, O_RDWR, &from_file_vnode, NULL);
	if(resp < 0) {
		return resp;
	}else { /* == 0*/
		if(S_ISDIR(from_file_vnode->vn_mode)) {
			vput(from_file_vnode);
			return -EISDIR;
		}
	}

	vnode_t* dir_vnode;
	size_t filename_len = 0;
	const char *filename;
	int dir_namev_retval = dir_namev(to, &filename_len, &filename, NULL, &dir_vnode);
	if(dir_namev_retval < 0) {
		vput(from_file_vnode);
		return dir_namev_retval;
	} else{ /* ==0*/
		vnode_t *to_file_vnode = NULL;
		int lookup_retval = lookup(dir_vnode, filename, filename_len, &to_file_vnode);
		if(lookup_retval == 0) { /* file found */
			vput(to_file_vnode);
			vput(dir_vnode);
			vput(from_file_vnode);
			return -EEXIST;
		}else { /* to file doesnt exists */
			vput(to_file_vnode);
			KASSERT(NULL != dir_vnode->vn_ops->link);
			return dir_vnode->vn_ops->link(from_file_vnode, dir_vnode, filename, filename_len);
		}
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
int do_rename(const char *oldname, const char *newname) {
	/*NOT_YET_IMPLEMENTED("VFS: do_rename");
	return -1;
	*/
	int link_resp = do_link(newname, oldname);
	if(link_resp < 0) {
		return link_resp;
	}
	int unlink_resp = do_unlink(oldname);
	return unlink_resp;
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
int do_chdir(const char *path) {
	/*
	NOT_YET_IMPLEMENTED("VFS: do_chdir");
	return -1;
	*/
	vnode_t* file_vnode;
	int file_vnode_resp = open_namev(path, O_RDWR, &file_vnode, NULL);
	if(file_vnode_resp < 0){
		return file_vnode_resp;
	}
	if(!S_ISDIR(file_vnode->vn_mode)){
		vput(file_vnode);
		return -ENOTDIR;
	}

	vnode_t* old_cwd = curproc->p_cwd;
	curproc->p_cwd = file_vnode; /*open_namev increments the ref count on the new cwd*/
	vput(old_cwd);
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
int do_getdent(int fd, struct dirent *dirp) {
	/*  NOT_YET_IMPLEMENTED("VFS: do_getdent");
	 return -1;*/
	if (fd == -1) { /* file obviously not open, -1 is not allowed for lseek unlike write*/
		/*need to find which test executes this code path*/
		return -EBADF;
	};
	file_t* file = fget(fd); /*fget increments file reference count if the file with this file descriptor exists*/
	if (file == NULL) { /* file descriptor not valid*/
		/*need to find which test executes this code path*/
		return -EBADF; /*or should we return -EBADF?*/
	};
	if (!S_ISDIR(file->f_vnode->vn_mode)) { /*file descriptor doesn't refer to directory*/
		/*need to find which test executes this code path*/
		fput(file);
		return -ENOTDIR;
	};
	KASSERT(file->f_vnode->vn_ops->readdir != NULL);
	/*need to find where it is tested*/
	vnode_t* v = file->f_vnode;
	int old_offset = file->f_pos;
	int new_offset = file->f_vnode->vn_ops->readdir(v, old_offset, dirp);
	file->f_pos = new_offset + old_offset;
	fput(file);
	/*vput(v);*/
	return 0;

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
int do_lseek(int fd, int offset, int whence) {
	/*  NOT_YET_IMPLEMENTED("VFS: do_lseek");
	 return -1;*/
	if (fd == -1) { /* file obviously not open, -1 is not allowed for lseek unlike write*/
		/*need to find which test executes this code path*/
		return -EBADF;
	}
	if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END) {
		/*need to find which test executes this code path*/
		return EINVAL; /* or -EINVAL?*/
	};
	file_t* file = fget(fd); /*fget increments file reference count if the file with this file descriptor exists*/
	if (file == NULL) {
		/*need to find which test executes this code path*/
		return -EBADF; /*or should we return -EBADF?*/
	};
	int file_len = file->f_vnode->vn_len; /*get the file length*/
	off_t old_offset = file->f_pos; /* get old file offset*/
	/*calculate new offset:*/
	int new_offset;
	switch (whence) {
	case SEEK_CUR:
		/*need to find which test executes this code path*/
		new_offset = old_offset + offset;
		break;
	case SEEK_END:
		/*need to find which test executes this code path*/
		new_offset = file_len + offset;
		break;
	case SEEK_SET:
		/*need to find which test executes this code path*/
		new_offset = offset;
		break;
	};
	if (new_offset < 0) { /*offset can actually be larger than file length, but can't be negative*/
		/*need to find which test executes this code path*/
		fput(file);
		return -EINVAL; /* or -EINVAL?*/
	};
	/*need to find which test executes this code path*/
	file->f_pos = new_offset;
	fput(file);
	return new_offset;
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
int do_stat(const char *path, struct stat *buf) {
	/*NOT_YET_IMPLEMENTED("VFS: do_stat");
	 return -1;*/
	vnode_t *node = NULL;
	if (strlen(path) == 0) { /*specified path doesn't exist*/
		/*need to find a test*/
		return -ENOENT;
	}
	if (path[0] == '/') { /*set the base dir to root file system dir*/
		/*get root vnode_t*/
		/*test*/
		vnode_t* root = curproc->p_cwd->vn_fs->fs_root;
		vref(root);
		int rst = lookup(root, path, strlen(path),&node);
		if(rst < 0) {/*this function should be implemented in namev.c*/
			/*test*/
			vput(root);
			return rst;
		};
		vput(root);
	} else {
		/*test*/
		vref(curproc->p_cwd);
		int rst = lookup(curproc->p_cwd, path, strlen(path), &node);
		if(rst < 0){
			/*test*/
			vput(curproc->p_cwd);
			return rst;
		};
		vput(curproc->p_cwd);
	}
	vnode_t* vnode = node; /*need to find it*/
	KASSERT(vnode->vn_ops->stat);
	dbg(DBG_PRINT, "(GRADING2A 3.f)\n");
	int result = vnode->vn_ops->stat(vnode, buf);
	vput(vnode);
	return result;
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
