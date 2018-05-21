#include <types.h>
#include <string.h>
#include <stdio.h>
#include <console.h>
#include <wait.h>
#include <sync.h>
#include <proc.h>
#include <sched.h>
#include <dev.h>
#include <vfs.h>
#include <iobuf.h>
#include <inode.h>
#include <unistd.h>
#include <error.h>
#include <assert.h>
#include <poll.h>

#define STDIN_BUFSIZE               4096

/* *
 * device stdin is also a abstract device built on console
 * console is binding with serial, cga
 *
 * the physic device is now
 *
 * stdin read:
 *		(user) call syscall:read --> (kernel) sys_read find it is a device file -->
 *		call device io --> stdin io --> stdin read from stdin buffer;
 *
 * stdin write:
 *		(hardware) input a Bit from serail or keyboard --> (kernel)system trap -->
 *		(kernel) COM or KBD trap --> (kernel) write a Bit to stdin buffer
 * */
static char stdin_buffer[STDIN_BUFSIZE];
static off_t p_rpos, p_wpos;
static wait_queue_t __wait_queue, *wait_queue = &__wait_queue;

/* *
 * dev_stdin_write - write a Bit to stdin
 *		this function can only be called while a COM or KBD trap happens
 *		read a Bit from serial or keyboard then write in stdin buffer
 * */
void dev_stdin_write(char c)
{
	bool intr_flag;
	if (c != '\0') {
		local_intr_save(intr_flag);
		{
			stdin_buffer[p_wpos % STDIN_BUFSIZE] = c;
			if (p_wpos - p_rpos < STDIN_BUFSIZE) {
				p_wpos++;
			}
			if (!wait_queue_empty(wait_queue)) {
				wakeup_queue(wait_queue, WT_KBD, 1);
			}
		}
		local_intr_restore(intr_flag);
	}
}

/* *
 * dev_stdin_read - read @len Bit to @buf from stdin
 *		there are two flag - p_rpos & p_wpos
 *		every read or write operation will increase them
 *		if p_rpos >= p_wpos, current process should be wait
 * */
static int dev_stdin_read(char *buf, size_t len, int io_flags)
{
	int ret = 0;
	bool intr_flag;
	local_intr_save(intr_flag);
	{
		while (1) {
			if (ret >= len)
				break;
try_again:
			if (p_rpos < p_wpos) {
				char c = stdin_buffer[p_rpos % STDIN_BUFSIZE];
				//FIXME
				cons_putc(c);
				*buf++ = c;
				p_rpos++;
				ret++;
				if (p_rpos >= p_wpos)
					break;
			} else {
        if(/*len == 1 && */(io_flags & O_NONBLOCK)) {
          ret = -E_AGAIN;
          //panic("%x", io_flags);
          break;
        }
				wait_t __wait, *wait = &__wait;
				wait_current_set(wait_queue, wait, WT_KBD);
				local_intr_restore(intr_flag);

				schedule();

				local_intr_save(intr_flag);
				wait_current_del(wait_queue, wait);
				if (wait->wakeup_flags == WT_KBD) {
					goto try_again;
				}
				break;
			}
		}
	}
	local_intr_restore(intr_flag);
	return ret;
}

static int stdin_poll(struct device *dev, wait_t *wait, int io_requests)
{
  if(io_requests | POLL_READ_AVAILABLE) {
    if(p_rpos < p_wpos) {
      return POLL_READ_AVAILABLE;
    }
    else {
      if(wait == NULL) return 0;
      //Since this "stdin" is not process independent, having 2 wait object in
      //the queue seems to be pretty weird.
      assert(wait_queue_empty(wait_queue));
      wait_queue_add(wait_queue, wait);
      return 0;
    }
  }
  else {
    return 0;
  }
}

static int stdin_open(struct device *dev, uint32_t open_flags)
{
	if (open_flags != O_RDONLY) {
		return -E_INVAL;
	}
	return 0;
}

static int stdin_close(struct device *dev)
{
	return 0;
}

/* *
 * for stdin io, write is invalidate
 * */
static int stdin_io(struct device *dev, struct iobuf *iob, bool write, int io_flags)
{
	if (!write) {
		int ret;
		if ((ret = dev_stdin_read(iob->io_base, iob->io_resid, io_flags)) > 0) {
			iob->io_resid -= ret;
		}
		return ret;
	}
	return -E_INVAL;
}

static int stdin_ioctl(struct device *dev, int op, void *data)
{
	return -E_INVAL;
}

static void stdin_device_init(struct device *dev)
{
	memset(dev, 0, sizeof(*dev));
	dev->d_blocks = 0;
	dev->d_blocksize = 1;
	dev->d_open = stdin_open;
	dev->d_close = stdin_close;
	dev->d_io = stdin_io;
	dev->d_ioctl = stdin_ioctl;
  dev->d_poll = stdin_poll;
	p_rpos = p_wpos = 0;
	wait_queue_init(wait_queue);
}

void dev_init_stdin(void)
{
	struct inode *node;
	if ((node = dev_create_inode()) == NULL) {
		panic("stdin: dev_create_node.\n");
	}
	stdin_device_init(vop_info(node, device));

	int ret;
	if ((ret = vfs_add_dev("stdin", node, 0)) != 0) {
		panic("stdin: vfs_add_dev: %e.\n", ret);
	}
}
