#include <types.h>
#include <string.h>
#include <slab.h>
#include <vfs.h>
#include <inode.h>
#include <pipe.h>
#include <pipe_state.h>
#include <iobuf.h>
#include <stat.h>
#include <poll.h>
#include <unistd.h>
#include <error.h>
#include <assert.h>
#include <pipe_state.h>

static int pipe_inode_open(struct inode *node, uint32_t open_flags)
{
	if (open_flags & (O_TRUNC | O_APPEND)) {
		return -E_INVAL;
	}
	struct pipe_inode *pin = vop_info(node, pipe_inode);
	switch (open_flags & O_ACCMODE) {
	case O_RDONLY:
		return (pin->pin_type == PIN_RDONLY) ? 0 : -E_INVAL;
	case O_WRONLY:
		return (pin->pin_type == PIN_WRONLY) ? 0 : -E_INVAL;
	default:
		return -E_INVAL;
	}
}

static int pipe_inode_close(struct inode *node)
{
	struct pipe_inode *pin = vop_info(node, pipe_inode);
	pipe_state_close(pin->state);
	return 0;
}

static int pipe_inode_read(struct inode *node, struct iobuf *iob, int io_flags)
{
  bool no_block = (io_flags & O_NONBLOCK) ? 1 : 0;
	struct pipe_inode *pin = vop_info(node, pipe_inode);
	if (pin->pin_type != PIN_RDONLY) {
		return -E_INVAL;
	}
	size_t ret;
	if ((ret =
	     pipe_state_read(pin->state, iob->io_base, iob->io_resid, no_block)) != 0) {
		iobuf_skip(iob, ret);
	}
	return 0;
}

static int pipe_inode_write(struct inode *node, struct iobuf *iob, int io_flags)
{
  bool no_block = (io_flags & O_NONBLOCK) ? 1 : 0;
	struct pipe_inode *pin = vop_info(node, pipe_inode);
	if (pin->pin_type != PIN_WRONLY) {
		return -E_INVAL;
	}
	size_t ret;
	if ((ret =
	     pipe_state_write(pin->state, iob->io_base, iob->io_resid, no_block)) != 0) {
		iobuf_skip(iob, ret);
	}
	return 0;
}

//TODO: Should be moved to a pipe_state function.
static int pipe_inode_poll(struct inode *node, wait_t *wait, int io_requests)
{
  struct pipe_inode *pipe_inode = vop_info(node, pipe_inode);
  struct pipe_state *state= pipe_inode->state;
  if(pipe_inode->pin_type == PIN_WRONLY) {
    if(io_requests | POLL_WRITE_AVAILABLE) {
      //TODO: This is ugly, should be using pipe_state macros.
      if (state->p_wpos - state->p_rpos >= PGSIZE - sizeof(struct pipe_state)) {
    		if (state->isclosed) {
    			return 0;
    		} else {
    			//unlock_state(state);
          if(wait != NULL) wait_queue_add(&state->writer_queue, wait);
          return 0;
    		}
    	}
      else {
        return POLL_WRITE_AVAILABLE;
      }
    }
    else {
      return 0;
    }
  }
  else {
    if(io_requests | POLL_READ_AVAILABLE) {
      //lock_state(state);
    	if (state->p_rpos == state->p_wpos) {
    		if (state->isclosed) {
    			return 0;
    		} else {
    			//unlock_state(state);
          if(wait != NULL) wait_queue_add(&state->reader_queue, wait);
          return 0;
    		}
    	}
      else {
        return POLL_READ_AVAILABLE;
      }
    }
    else {
      return 0;
    }
  }
}

static int pipe_inode_fstat(struct inode *node, struct stat *stat)
{
	int ret;
	memset(stat, 0, sizeof(struct stat));
	if ((ret = vop_gettype(node, &(stat->st_mode))) != 0) {
		return ret;
	}
	struct pipe_inode *pin = vop_info(node, pipe_inode);
	stat->st_nlinks = 1;
	stat->st_blocks = 0;
	stat->st_size =
	    pipe_state_size(pin->state, pin->pin_type == PIN_WRONLY);
	return 0;
}

static int pipe_inode_namefile(struct inode *node, struct iobuf *iob)
{
	struct pipe_inode *pin = vop_info(node, pipe_inode);
	size_t len = (pin->name != NULL) ? strlen(pin->name) : 0;
	if (iob->io_resid < len + 1) {
		return -E_NO_MEM;
	}
	if (pin->name != NULL) {
		memcpy(iob->io_base, pin->name, len);
	}
	((char *)(iob->io_base))[len++] = '\0';
	iobuf_skip(iob, len);
	return 0;
}

static int pipe_inode_reclaim(struct inode *node)
{
	struct pipe_inode *pin = vop_info(node, pipe_inode);
	if (pin->name != NULL) {
		struct pipe_fs *pipe = fsop_info(vop_fs(node), pipe);
		lock_pipe(pipe);
		assert(pin->reclaim_count > 0);
		if ((--pin->reclaim_count) != 0 || inode_ref_count(node) != 0) {
			unlock_pipe(pipe);
			return -E_BUSY;
		}
		list_del(&(pin->pipe_link));
		unlock_pipe(pipe);
		kfree(pin->name);
	}
	pipe_state_release(pin->state);
	vop_kill(node);
	return 0;
}

static int pipe_inode_gettype(struct inode *node, uint32_t * type_store)
{
	*type_store = S_IFCHR;
	return 0;
}

static int pipe_inode_ioctl(struct inode * node, int op, void *data)
{
  struct pipe_inode *pin = vop_info(node, pipe_inode);
  if(op & 04000) pin->no_block = 1;
  else pin->no_block = 0;
  return 0;
}

static const struct inode_ops pipe_node_ops = {
	.vop_magic = VOP_MAGIC,
	.vop_open = pipe_inode_open,
	.vop_close = pipe_inode_close,
	.vop_read = pipe_inode_read,
	.vop_write = pipe_inode_write,
	.vop_fstat = pipe_inode_fstat,
	.vop_fsync = NULL_VOP_PASS,
	.vop_mkdir = NULL_VOP_NOTDIR,
	.vop_link = NULL_VOP_NOTDIR,
	.vop_rename = NULL_VOP_NOTDIR,
	.vop_readlink = NULL_VOP_INVAL,
	.vop_symlink = NULL_VOP_NOTDIR,
	.vop_namefile = pipe_inode_namefile,
	.vop_getdirentry = NULL_VOP_INVAL,
	.vop_reclaim = pipe_inode_reclaim,
	.vop_ioctl = pipe_inode_ioctl,
	.vop_gettype = pipe_inode_gettype,
	.vop_tryseek = NULL_VOP_INVAL,
	.vop_truncate = NULL_VOP_INVAL,
	.vop_create = NULL_VOP_NOTDIR,
	.vop_unlink = NULL_VOP_NOTDIR,
	.vop_lookup = NULL_VOP_NOTDIR,
	.vop_lookup_parent = NULL_VOP_NOTDIR,
  .vop_poll = pipe_inode_poll,
};

static void
pipe_inode_init(struct pipe_inode *pin, char *name, struct pipe_state *state,
		bool readonly)
{
	assert(state != NULL);
	pin->pin_type = readonly ? PIN_RDONLY : PIN_WRONLY;
	pin->name = name, pin->state = state, pin->reclaim_count = 1;
  pin->no_block = 0;
	list_init(&(pin->pipe_link));
}

struct inode *pipe_create_inode(struct fs *fs, const char *__name,
				struct pipe_state *state, bool readonly)
{
	char *name = NULL;
	if (__name == NULL || (name = strdup(__name)) != NULL) {
		struct inode *node;
		if ((node = alloc_inode(pipe_inode)) != NULL) {
			vop_init(node, &pipe_node_ops, fs);
			pipe_inode_init(vop_info(node, pipe_inode), name, state,
					readonly);
			return node;
		}
		if (name != NULL) {
			kfree(name);
		}
	}
	return NULL;
}

int pipe_open(struct inode **rnode_store, struct inode **wnode_store)
{
	int ret;
	struct inode *root;
	if ((ret = vfs_get_root("pipe", &root)) != 0) {
		return ret;
	}
	ret = -E_NO_MEM;

	struct pipe_state *state;
	if ((state = pipe_state_create()) == NULL) {
		goto out;
	}

	struct fs *fs = vop_fs(root);
	struct inode *node[2] = { NULL, NULL };
	if ((node[0] = pipe_create_inode(fs, NULL, state, 1)) == NULL) {
		goto failed_cleanup_state;
	}

	pipe_state_acquire(state);
	if ((node[1] = pipe_create_inode(fs, NULL, state, 0)) == NULL) {
		goto failed_cleanup_node0;
	}

	vop_open_inc(node[0]), vop_open_inc(node[1]);

	*rnode_store = node[0];
	*wnode_store = node[1];
	ret = 0;

out:
	vop_ref_dec(root);
	return ret;

failed_cleanup_node0:
	vop_ref_dec(node[0]);
failed_cleanup_state:
	pipe_state_release(state);
	goto out;
}
