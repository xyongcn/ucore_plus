#ifndef __KERN_FS_PIPE_PIPE_STATE_H__
#define __KERN_FS_PIPE_PIPE_STATE_H__

struct pipe_state;

struct pipe_state {
	off_t p_rpos;
	off_t p_wpos;
	uint8_t *buf;
	bool isclosed;
	int ref_count;
	semaphore_t sem;
	wait_queue_t reader_queue;
	wait_queue_t writer_queue;
};

struct pipe_state *pipe_state_create(void);
void pipe_state_acquire(struct pipe_state *state);
void pipe_state_release(struct pipe_state *state);
void pipe_state_close(struct pipe_state *state);

size_t pipe_state_size(struct pipe_state *state, bool write);
size_t pipe_state_read(struct pipe_state *state, void *buf, size_t n, bool no_block);
size_t pipe_state_write(struct pipe_state *state, void *buf, size_t n, bool no_block);

#endif /* !__KERN_FS_PIPE_PIPE_STATE_H__ */
