#include <proc.h>
#include <syscall.h>
#include <trap.h>
#include <stdio.h>
#include <pmm.h>
#include <clock.h>
#include <error.h>
#include <assert.h>
#include <sem.h>
#include <event.h>
#include <mbox.h>
#include <stat.h>
#include <dirent.h>
#include <sysfile.h>
#include <kio.h>

static uint64_t sys_exit(uint64_t arg[])
{
	int error_code = (int)arg[0];
	return do_exit(error_code);
}

static uint64_t sys_fork(uint64_t arg[])
{
	struct trapframe *tf = current->tf;
	uintptr_t stack = tf->tf_rsp;
	return do_fork(0, stack, tf);
}

static uint64_t sys_wait(uint64_t arg[])
{
	int pid = (int)arg[0];
	int *store = (int *)arg[1];
	return do_wait(pid, store);
}

static uint64_t sys_exec(uint64_t arg[])
{
	const char *name = (const char *)arg[0];
	const char **argv = (const char **)arg[1];
	const char **envp = (const char **)arg[2];
	return do_execve(name, argv, envp);
}

static uint64_t sys_clone(uint64_t arg[])
{
	struct trapframe *tf = current->tf;
	uint32_t clone_flags = (uint32_t) arg[0];
	uintptr_t stack = (uintptr_t) arg[1];
	if (stack == 0) {
		stack = tf->tf_rsp;
	}
	return do_fork(clone_flags, stack, tf);
}

static uint64_t sys_exit_thread(uint64_t arg[])
{
	int error_code = (int)arg[0];
	return do_exit_thread(error_code);
}

static uint64_t sys_yield(uint64_t arg[])
{
	return do_yield();
}

static uint64_t sys_sleep(uint64_t arg[])
{
	unsigned int time = (unsigned int)arg[0];
	return do_sleep(time);
}

static uint64_t sys_kill(uint64_t arg[])
{
	int pid = (int)arg[0];
	return do_kill(pid, -E_KILLED);
}

static uint64_t sys_gettime(uint64_t arg[])
{
	return (int)ticks;
}

static uint64_t sys_getpid(uint64_t arg[])
{
	return current->pid;
}

static uint64_t sys_brk(uint64_t arg[])
{
	uintptr_t *brk_store = (uintptr_t *) arg[0];
	return do_brk(brk_store);
}

static uint64_t sys_mmap(uint64_t arg[])
{
	uintptr_t *addr_store = (uintptr_t *) arg[0];
	size_t len = (size_t) arg[1];
	uint32_t mmap_flags = (uint32_t) arg[2];
	return do_mmap(addr_store, len, mmap_flags);
}

static uint64_t sys_munmap(uint64_t arg[])
{
	uintptr_t addr = (uintptr_t) arg[0];
	size_t len = (size_t) arg[1];
	return do_munmap(addr, len);
}

static uint64_t sys_shmem(uint64_t arg[])
{
	uintptr_t *addr_store = (uintptr_t *) arg[0];
	size_t len = (size_t) arg[1];
	uint32_t mmap_flags = (uint32_t) arg[2];
	return do_shmem(addr_store, len, mmap_flags);
}

static uint64_t sys_putc(uint64_t arg[])
{
	int c = (int)arg[0];
	cons_putc(c);
	return 0;
}

static uint64_t sys_pgdir(uint64_t arg[])
{
	print_pgdir(kprintf);
	return 0;
}

static uint64_t sys_sem_init(uint64_t arg[])
{
	int value = (int)arg[0];
	return ipc_sem_init(value);
}

static uint64_t sys_sem_post(uint64_t arg[])
{
	sem_t sem_id = (sem_t) arg[0];
	return ipc_sem_post(sem_id);
}

static uint64_t sys_sem_wait(uint64_t arg[])
{
	sem_t sem_id = (sem_t) arg[0];
	unsigned int timeout = (unsigned int)arg[1];
	return ipc_sem_wait(sem_id, timeout);
}

static uint64_t sys_sem_free(uint64_t arg[])
{
	sem_t sem_id = (sem_t) arg[0];
	return ipc_sem_free(sem_id);
}

static uint64_t sys_sem_get_value(uint64_t arg[])
{
	sem_t sem_id = (sem_t) arg[0];
	int *value_store = (int *)arg[1];
	return ipc_sem_get_value(sem_id, value_store);
}

static uint64_t sys_event_send(uint64_t arg[])
{
	int pid = (int)arg[0];
	int event = (int)arg[1];
	unsigned int timeout = (unsigned int)arg[2];
	return ipc_event_send(pid, event, timeout);
}

static uint64_t sys_event_recv(uint64_t arg[])
{
	int *pid_store = (int *)arg[0];
	int *event_store = (int *)arg[1];
	unsigned int timeout = (unsigned int)arg[2];
	return ipc_event_recv(pid_store, event_store, timeout);
}

static uint64_t sys_mbox_init(uint64_t arg[])
{
	unsigned int max_slots = (unsigned int)arg[0];
	return ipc_mbox_init(max_slots);
}

static uint64_t sys_mbox_send(uint64_t arg[])
{
	int id = (int)arg[0];
	struct mboxbuf *buf = (struct mboxbuf *)arg[1];
	unsigned int timeout = (unsigned int)arg[2];
	return ipc_mbox_send(id, buf, timeout);
}

static uint64_t sys_mbox_recv(uint64_t arg[])
{
	int id = (int)arg[0];
	struct mboxbuf *buf = (struct mboxbuf *)arg[1];
	unsigned int timeout = (unsigned int)arg[2];
	return ipc_mbox_recv(id, buf, timeout);
}

static uint64_t sys_mbox_free(uint64_t arg[])
{
	int id = (int)arg[0];
	return ipc_mbox_free(id);
}

static uint64_t sys_mbox_info(uint64_t arg[])
{
	int id = (int)arg[0];
	struct mboxinfo *info = (struct mboxinfo *)arg[1];
	return ipc_mbox_info(id, info);
}

static uint64_t sys_open(uint64_t arg[])
{
	const char *path = (const char *)arg[0];
	uint32_t open_flags = (uint32_t) arg[1];
	return sysfile_open(path, open_flags);
}

static uint64_t sys_close(uint64_t arg[])
{
	int fd = (int)arg[0];
	return sysfile_close(fd);
}

static uint64_t sys_read(uint64_t arg[])
{
	int fd = (int)arg[0];
	void *base = (void *)arg[1];
	size_t len = (size_t) arg[2];
	return sysfile_read(fd, base, len);
}

static uint64_t sys_write(uint64_t arg[])
{
	int fd = (int)arg[0];
	void *base = (void *)arg[1];
	size_t len = (size_t) arg[2];
	return sysfile_write(fd, base, len);
}

static uint64_t sys_seek(uint64_t arg[])
{
	int fd = (int)arg[0];
	off_t pos = (off_t) arg[1];
	int whence = (int)arg[2];
	return sysfile_seek(fd, pos, whence);
}

static uint64_t sys_fstat(uint64_t arg[])
{
	int fd = (int)arg[0];
	struct stat *stat = (struct stat *)arg[1];
	return sysfile_fstat(fd, stat);
}

static uint64_t sys_fsync(uint64_t arg[])
{
	int fd = (int)arg[0];
	return sysfile_fsync(fd);
}

static uint64_t sys_chdir(uint64_t arg[])
{
	const char *path = (const char *)arg[0];
	return sysfile_chdir(path);
}

static uint64_t sys_getcwd(uint64_t arg[])
{
	char *buf = (char *)arg[0];
	size_t len = (size_t) arg[1];
	return sysfile_getcwd(buf, len);
}

static uint64_t sys_mkdir(uint64_t arg[])
{
	const char *path = (const char *)arg[0];
	return sysfile_mkdir(path);
}

static uint64_t sys_link(uint64_t arg[])
{
	const char *path1 = (const char *)arg[0];
	const char *path2 = (const char *)arg[1];
	return sysfile_link(path1, path2);
}

static uint64_t sys_rename(uint64_t arg[])
{
	const char *path1 = (const char *)arg[0];
	const char *path2 = (const char *)arg[1];
	return sysfile_rename(path1, path2);
}

static uint64_t sys_unlink(uint64_t arg[])
{
	const char *name = (const char *)arg[0];
	return sysfile_unlink(name);
}

static uint64_t sys_getdirentry(uint64_t arg[])
{
	int fd = (int)arg[0];
	struct dirent *direntp = (struct dirent *)arg[1];
	return sysfile_getdirentry(fd, direntp, NULL);
}

static uint64_t sys_dup(uint64_t arg[])
{
	int fd1 = (int)arg[0];
	int fd2 = (int)arg[1];
	return sysfile_dup(fd1, fd2);
}

static uint64_t sys_pipe(uint64_t arg[])
{
	int *fd_store = (int *)arg[0];
	return sysfile_pipe(fd_store);
}

static uint64_t sys_mkfifo(uint64_t arg[])
{
	const char *name = (const char *)arg[0];
	uint32_t open_flags = (uint32_t) arg[1];
	return sysfile_mkfifo(name, open_flags);
}

static uint64_t sys_halt(uint64_t arg[])
{
	do_halt();
	panic("halt returned");
}

static uint64_t(*syscalls[]) (uint64_t arg[]) = {
[SYS_exit] sys_exit,
	    [SYS_fork] sys_fork,
	    [SYS_wait] sys_wait,
	    [SYS_exec] sys_exec,
	    [SYS_clone] sys_clone,
	    [SYS_exit_thread] sys_exit_thread,
	    [SYS_yield] sys_yield,
	    [SYS_kill] sys_kill,
	    [SYS_sleep] sys_sleep,
	    [SYS_gettime] sys_gettime,
	    [SYS_getpid] sys_getpid,
	    [SYS_brk] sys_brk,
	    [SYS_mmap] sys_mmap,
	    [SYS_munmap] sys_munmap,
	    [SYS_shmem] sys_shmem,
	    [SYS_putc] sys_putc,
	    [SYS_pgdir] sys_pgdir,
	    [SYS_sem_init] sys_sem_init,
	    [SYS_sem_post] sys_sem_post,
	    [SYS_sem_wait] sys_sem_wait,
	    [SYS_sem_free] sys_sem_free,
	    [SYS_sem_get_value] sys_sem_get_value,
	    [SYS_event_send] sys_event_send,
	    [SYS_event_recv] sys_event_recv,
	    [SYS_mbox_init] sys_mbox_init,
	    [SYS_mbox_send] sys_mbox_send,
	    [SYS_mbox_recv] sys_mbox_recv,
	    [SYS_mbox_free] sys_mbox_free,
	    [SYS_mbox_info] sys_mbox_info,
	    [SYS_open] sys_open,
	    [SYS_close] sys_close,
	    [SYS_read] sys_read,
	    [SYS_write] sys_write,
	    [SYS_seek] sys_seek,
	    [SYS_fstat] sys_fstat,
	    [SYS_fsync] sys_fsync,
	    [SYS_chdir] sys_chdir,
	    [SYS_getcwd] sys_getcwd,
	    [SYS_mkdir] sys_mkdir,
	    [SYS_link] sys_link,
	    [SYS_rename] sys_rename,
	    [SYS_unlink] sys_unlink,
	    [SYS_getdirentry] sys_getdirentry,
	    [SYS_dup] sys_dup,[SYS_pipe] sys_pipe,[SYS_mkfifo] sys_mkfifo,
            [SYS_halt] sys_halt,};

#define NUM_SYSCALLS        ((sizeof(syscalls)) / (sizeof(syscalls[0])))

void syscall(void)
{
	struct trapframe *tf = current->tf;
	uint64_t arg[6];
	int num = tf->tf_regs.reg_rax;
	if (num >= 0 && num < NUM_SYSCALLS) {
		if (syscalls[num] != NULL) {
//if(num==SYS_exit || num==SYS_write)	 kprintf("syscall %d, pid = %d, name = %s  cs 0x%x.\n", num, current->pid, current->name, tf->tf_cs);
			arg[0] = tf->tf_regs.reg_rdi;
			arg[1] = tf->tf_regs.reg_rsi;
			arg[2] = tf->tf_regs.reg_rdx;
			arg[3] = tf->tf_regs.reg_rcx;
			arg[4] = tf->tf_regs.reg_r8;
			arg[5] = tf->tf_regs.reg_r9;
			tf->tf_regs.reg_rax = syscalls[num] (arg);
			return;
		}
	}
	print_trapframe(tf);
	panic("undefined syscall %d, pid = %d, name = %s.\n",
	      num, current->pid, current->name);
}

#include "unistd_64.h"

static uint64_t sys_ioctl(uint64_t arg[])
{
	int i;
	kprintf("syscall_ioctl with args :");
	for (i = 0; i < 6; i++)
		kprintf(" %x", arg[i]);
	kprintf("\n");
	return 0;
}

uint64_t unknown(uint64_t arg[])
{
	panic("unknown LINUX syscall\n");
}

static uint64_t(*syscalls_linux[305]) (uint64_t arg[]);

#define NUM_LINUX_SYSCALLS        ((sizeof(syscalls_linux)) / (sizeof(syscalls_linux[0])))

void syscall_linux()
{
//	panic("Syscall Linux\n");
//prrsp();
	struct trapframe *tf = current->tf;
	uint64_t arg[6];
	int num = tf->tf_regs.reg_rax;
	if (num >= 0 && num < NUM_LINUX_SYSCALLS) {
		if (syscalls_linux[num] != unknown)
		if (syscalls_linux[num] != NULL) {
	 kprintf("LINUX syscall %d, pid = %d, name = %s  cs 0x%x.\n", num, current->pid, current->name, tf->tf_cs);
			arg[0] = tf->tf_regs.reg_rdi;
			arg[1] = tf->tf_regs.reg_rsi;
			arg[2] = tf->tf_regs.reg_rdx;
			arg[3] = tf->tf_regs.reg_r10;
			arg[4] = tf->tf_regs.reg_r8;
			arg[5] = tf->tf_regs.reg_r9;
			tf->tf_regs.reg_rax = syscalls_linux[num] (arg);
	kprintf("syscall return %d.\n", tf->tf_regs.reg_rax);
			return;
		}
	}
	print_trapframe(tf);
	panic("undefined LINUX syscall %d, pid = %d, name = %s.\n",
	      num, current->pid, current->name);
	tf->tf_regs.reg_rax = 0;
}

static uint64_t(*syscalls_linux[305]) (uint64_t arg[]) = {
	[__NR_read] sys_read,
	[__NR_write] sys_write,
	[__NR_open] unknown,
	[__NR_close] unknown,
	[__NR_stat] unknown,
	[__NR_fstat] unknown,
	[__NR_lstat] unknown,
	[__NR_poll] unknown,
	[__NR_lseek] unknown,
	[__NR_mmap] unknown,
	[__NR_mprotect] unknown,
	[__NR_munmap] unknown,
	[__NR_brk] unknown,
	[__NR_rt_sigaction] unknown,
	[__NR_rt_sigprocmask] unknown,
	[__NR_rt_sigreturn] unknown,
	[__NR_ioctl] sys_ioctl,
	[__NR_pread64] unknown,
	[__NR_pwrite64] unknown,
	[__NR_readv] unknown,
	[__NR_writev] unknown,
	[__NR_access] unknown,
	[__NR_pipe] unknown,
	[__NR_select] unknown,
	[__NR_sched_yield] unknown,
	[__NR_mremap] unknown,
	[__NR_msync] unknown,
	[__NR_mincore] unknown,
	[__NR_madvise] unknown,
	[__NR_shmget] unknown,
	[__NR_shmat] unknown,
	[__NR_shmctl] unknown,
	[__NR_dup] unknown,
	[__NR_dup2] unknown,
	[__NR_pause] unknown,
	[__NR_nanosleep] unknown,
	[__NR_getitimer] unknown,
	[__NR_alarm] unknown,
	[__NR_setitimer] unknown,
	[__NR_getpid] unknown,
	[__NR_sendfile] unknown,
	[__NR_socket] unknown,
	[__NR_connect] unknown,
	[__NR_accept] unknown,
	[__NR_sendto] unknown,
	[__NR_recvfrom] unknown,
	[__NR_sendmsg] unknown,
	[__NR_recvmsg] unknown,
	[__NR_shutdown] unknown,
	[__NR_bind] unknown,
	[__NR_listen] unknown,
	[__NR_getsockname] unknown,
	[__NR_getpeername] unknown,
	[__NR_socketpair] unknown,
	[__NR_setsockopt] unknown,
	[__NR_getsockopt] unknown,
	[__NR_clone] unknown,
	[__NR_fork] unknown,
	[__NR_vfork] unknown,
	[__NR_execve] unknown,
	[__NR_exit] sys_exit,
	[__NR_wait4] unknown,
	[__NR_kill] unknown,
	[__NR_uname] unknown,
	[__NR_semget] unknown,
	[__NR_semop] unknown,
	[__NR_semctl] unknown,
	[__NR_shmdt] unknown,
	[__NR_msgget] unknown,
	[__NR_msgsnd] unknown,
	[__NR_msgrcv] unknown,
	[__NR_msgctl] unknown,
	[__NR_fcntl] unknown,
	[__NR_flock] unknown,
	[__NR_fsync] unknown,
	[__NR_fdatasync] unknown,
	[__NR_truncate] unknown,
	[__NR_ftruncate] unknown,
	[__NR_getdents] unknown,
	[__NR_getcwd] unknown,
	[__NR_chdir] unknown,
	[__NR_fchdir] unknown,
	[__NR_rename] unknown,
	[__NR_mkdir] unknown,
	[__NR_rmdir] unknown,
	[__NR_creat] unknown,
	[__NR_link] unknown,
	[__NR_unlink] unknown,
	[__NR_symlink] unknown,
	[__NR_readlink] unknown,
	[__NR_chmod] unknown,
	[__NR_fchmod] unknown,
	[__NR_chown] unknown,
	[__NR_fchown] unknown,
	[__NR_lchown] unknown,
	[__NR_umask] unknown,
	[__NR_gettimeofday] unknown,
	[__NR_getrlimit] unknown,
	[__NR_getrusage] unknown,
	[__NR_sysinfo] unknown,
	[__NR_times] unknown,
	[__NR_ptrace] unknown,
	[__NR_getuid] unknown,
	[__NR_syslog] unknown,
	[__NR_getgid] unknown,
	[__NR_setuid] unknown,
	[__NR_setgid] unknown,
	[__NR_geteuid] unknown,
	[__NR_getegid] unknown,
	[__NR_setpgid] unknown,
	[__NR_getppid] unknown,
	[__NR_getpgrp] unknown,
	[__NR_setsid] unknown,
	[__NR_setreuid] unknown,
	[__NR_setregid] unknown,
	[__NR_getgroups] unknown,
	[__NR_setgroups] unknown,
	[__NR_setresuid] unknown,
	[__NR_getresuid] unknown,
	[__NR_setresgid] unknown,
	[__NR_getresgid] unknown,
	[__NR_getpgid] unknown,
	[__NR_setfsuid] unknown,
	[__NR_setfsgid] unknown,
	[__NR_getsid] unknown,
	[__NR_capget] unknown,
	[__NR_capset] unknown,
	[__NR_rt_sigpending] unknown,
	[__NR_rt_sigtimedwait] unknown,
	[__NR_rt_sigqueueinfo] unknown,
	[__NR_rt_sigsuspend] unknown,
	[__NR_sigaltstack] unknown,
	[__NR_utime] unknown,
	[__NR_mknod] unknown,
	[__NR_uselib] unknown,
	[__NR_personality] unknown,
	[__NR_ustat] unknown,
	[__NR_statfs] unknown,
	[__NR_fstatfs] unknown,
	[__NR_sysfs] unknown,
	[__NR_getpriority] unknown,
	[__NR_setpriority] unknown,
	[__NR_sched_setparam] unknown,
	[__NR_sched_getparam] unknown,
	[__NR_sched_setscheduler] unknown,
	[__NR_sched_getscheduler] unknown,
	[__NR_sched_get_priority_max] unknown,
	[__NR_sched_get_priority_min] unknown,
	[__NR_sched_rr_get_interval] unknown,
	[__NR_mlock] unknown,
	[__NR_munlock] unknown,
	[__NR_mlockall] unknown,
	[__NR_munlockall] unknown,
	[__NR_vhangup] unknown,
	[__NR_modify_ldt] unknown,
	[__NR_pivot_root] unknown,
	[__NR__sysctl] unknown,
	[__NR_prctl] unknown,
	[__NR_arch_prctl] unknown,
	[__NR_adjtimex] unknown,
	[__NR_setrlimit] unknown,
	[__NR_chroot] unknown,
	[__NR_sync] unknown,
	[__NR_acct] unknown,
	[__NR_settimeofday] unknown,
	[__NR_mount] unknown,
	[__NR_umount2] unknown,
	[__NR_swapon] unknown,
	[__NR_swapoff] unknown,
	[__NR_reboot] unknown,
	[__NR_sethostname] unknown,
	[__NR_setdomainname] unknown,
	[__NR_iopl] unknown,
	[__NR_ioperm] unknown,
	[__NR_create_module] unknown,
	[__NR_init_module] unknown,
	[__NR_delete_module] unknown,
	[__NR_get_kernel_syms] unknown,
	[__NR_query_module] unknown,
	[__NR_quotactl] unknown,
	[__NR_nfsservctl] unknown,
	[__NR_getpmsg] unknown,
	[__NR_putpmsg] unknown,
	[__NR_afs_syscall] unknown,
	[__NR_tuxcall] unknown,
	[__NR_security] unknown,
	[__NR_gettid] unknown,
	[__NR_readahead] unknown,
	[__NR_setxattr] unknown,
	[__NR_lsetxattr] unknown,
	[__NR_fsetxattr] unknown,
	[__NR_getxattr] unknown,
	[__NR_lgetxattr] unknown,
	[__NR_fgetxattr] unknown,
	[__NR_listxattr] unknown,
	[__NR_llistxattr] unknown,
	[__NR_flistxattr] unknown,
	[__NR_removexattr] unknown,
	[__NR_lremovexattr] unknown,
	[__NR_fremovexattr] unknown,
	[__NR_tkill] unknown,
	[__NR_time] unknown,
	[__NR_futex] unknown,
	[__NR_sched_setaffinity] unknown,
	[__NR_sched_getaffinity] unknown,
	[__NR_set_thread_area] unknown,
	[__NR_io_setup] unknown,
	[__NR_io_destroy] unknown,
	[__NR_io_getevents] unknown,
	[__NR_io_submit] unknown,
	[__NR_io_cancel] unknown,
	[__NR_get_thread_area] unknown,
	[__NR_lookup_dcookie] unknown,
	[__NR_epoll_create] unknown,
	[__NR_epoll_ctl_old] unknown,
	[__NR_epoll_wait_old] unknown,
	[__NR_remap_file_pages] unknown,
	[__NR_getdents64] unknown,
	[__NR_set_tid_address] unknown,
	[__NR_restart_syscall] unknown,
	[__NR_semtimedop] unknown,
	[__NR_fadvise64] unknown,
	[__NR_timer_create] unknown,
	[__NR_timer_settime] unknown,
	[__NR_timer_gettime] unknown,
	[__NR_timer_getoverrun] unknown,
	[__NR_timer_delete] unknown,
	[__NR_clock_settime] unknown,
	[__NR_clock_gettime] unknown,
	[__NR_clock_getres] unknown,
	[__NR_clock_nanosleep] unknown,
	[__NR_exit_group] unknown,
	[__NR_epoll_wait] unknown,
	[__NR_epoll_ctl] unknown,
	[__NR_tgkill] unknown,
	[__NR_utimes] unknown,
	[__NR_vserver] unknown,
	[__NR_mbind] unknown,
	[__NR_set_mempolicy] unknown,
	[__NR_get_mempolicy] unknown,
	[__NR_mq_open] unknown,
	[__NR_mq_unlink] unknown,
	[__NR_mq_timedsend] unknown,
	[__NR_mq_timedreceive] unknown,
	[__NR_mq_notify] unknown,
	[__NR_mq_getsetattr] unknown,
	[__NR_kexec_load] unknown,
	[__NR_waitid] unknown,
	[__NR_add_key] unknown,
	[__NR_request_key] unknown,
	[__NR_keyctl] unknown,
	[__NR_ioprio_set] unknown,
	[__NR_ioprio_get] unknown,
	[__NR_inotify_init] unknown,
	[__NR_inotify_add_watch] unknown,
	[__NR_inotify_rm_watch] unknown,
	[__NR_migrate_pages] unknown,
	[__NR_openat] unknown,
	[__NR_mkdirat] unknown,
	[__NR_mknodat] unknown,
	[__NR_fchownat] unknown,
	[__NR_futimesat] unknown,
	[__NR_newfstatat] unknown,
	[__NR_unlinkat] unknown,
	[__NR_renameat] unknown,
	[__NR_linkat] unknown,
	[__NR_symlinkat] unknown,
	[__NR_readlinkat] unknown,
	[__NR_fchmodat] unknown,
	[__NR_faccessat] unknown,
	[__NR_pselect6] unknown,
	[__NR_ppoll] unknown,
	[__NR_unshare] unknown,
	[__NR_set_robust_list] unknown,
	[__NR_get_robust_list] unknown,
	[__NR_splice] unknown,
	[__NR_tee] unknown,
	[__NR_sync_file_range] unknown,
	[__NR_vmsplice] unknown,
	[__NR_move_pages] unknown,
	[__NR_utimensat] unknown,
	[__NR_epoll_pwait] unknown,
	[__NR_signalfd] unknown,
	[__NR_timerfd_create] unknown,
	[__NR_eventfd] unknown,
	[__NR_fallocate] unknown,
	[__NR_timerfd_settime] unknown,
	[__NR_timerfd_gettime] unknown,
	[__NR_accept4] unknown,
	[__NR_signalfd4] unknown,
	[__NR_eventfd2] unknown,
	[__NR_epoll_create1] unknown,
	[__NR_dup3] unknown,
	[__NR_pipe2] unknown,
	[__NR_inotify_init1] unknown,
	[__NR_preadv] unknown,
	[__NR_pwritev] unknown,
	[__NR_rt_tgsigqueueinfo] unknown,
	[__NR_perf_event_open] unknown,
	[__NR_recvmmsg] unknown,
	[__NR_fanotify_init] unknown,
	[__NR_fanotify_mark] unknown,
	[__NR_prlimit64] unknown
};


