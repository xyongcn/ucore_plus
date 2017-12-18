#include <unistd.h>
#include <proc.h>
#include <syscall.h>
#include <linux_unistd.h>
#include <syscall_linux.h>
#include <trap.h>
#include <stdio.h>
#include <pmm.h>
#include <assert.h>
#include <stat.h>
#include <dirent.h>
#include <sysfile.h>
#include <error.h>

//#define current (pls_read(current))

extern volatile int ticks;

static int sys_exit(uint32_t arg[])
{
	int error_code = (int)arg[0];
	return do_exit(error_code);
}

static int sys_fork(uint32_t arg[])
{
	struct trapframe *tf = current->tf;
	uintptr_t stack = tf->tf_regs.reg_r[MIPS_REG_SP];
	return do_fork(0, stack, tf);
}

static int sys_wait(uint32_t arg[])
{
	int pid = (int)arg[0];
	int *store = (int *)arg[1];
	return do_wait(pid, store);
}

static int sys_exec(uint32_t arg[])
{
	const char *name = (const char *)arg[0];
	const char **argc = (const char **)arg[1];
	const char **argv = (const char **)arg[2];
	return do_execve(name, argc, argv);
}

static int sys_yield(uint32_t arg[])
{
	return do_yield();
}

static int sys_kill(uint32_t arg[])
{
	int pid = (int)arg[0];
	return do_kill(pid, -E_KILLED);
}

static int sys_getpid(uint32_t arg[])
{
	return current->pid;
}

static int sys_brk(uint32_t arg[])
{
	uintptr_t *brk_store = (uintptr_t *) arg[0];
	return do_brk(brk_store);
}

static int sys_putc(uint32_t arg[])
{
	int c = (int)arg[0];
	kputchar(c);
	return 0;
}

static int sys_pgdir(uint32_t arg[])
{
	print_pgdir();
	return 0;
}

static int sys_gettime(uint32_t arg[])
{
	return (int)ticks;
}

static int sys_sleep(uint32_t arg[])
{
	unsigned int time = (unsigned int)arg[0];
	return do_sleep(time);
}

static int sys_open(uint32_t arg[])
{
	const char *path = (const char *)arg[0];
	uint32_t open_flags = (uint32_t) arg[1];
	return sysfile_open(path, open_flags);
}

static int sys_close(uint32_t arg[])
{
	int fd = (int)arg[0];
	return sysfile_close(fd);
}

static int sys_read(uint32_t arg[])
{
	int fd = (int)arg[0];
	void *base = (void *)arg[1];
	size_t len = (size_t) arg[2];
	return sysfile_read(fd, base, len);
}

static int sys_write(uint32_t arg[])
{
	int fd = (int)arg[0];
	void *base = (void *)arg[1];
	size_t len = (size_t) arg[2];
	return sysfile_write(fd, base, len);
}

static int sys_seek(uint32_t arg[])
{
	int fd = (int)arg[0];
	off_t pos = (off_t) arg[1];
	int whence = (int)arg[2];
	return sysfile_seek(fd, pos, whence);
}

static int sys_fstat(uint32_t arg[])
{
	int fd = (int)arg[0];
	struct stat *stat = (struct stat *)arg[1];
	return sysfile_fstat(fd, stat);
}

static int sys_fsync(uint32_t arg[])
{
	int fd = (int)arg[0];
	return sysfile_fsync(fd);
}

static int sys_chdir(uint32_t arg[])
{
	const char *path = (const char *)arg[0];
	return sysfile_chdir(path);
}

static int sys_getcwd(uint32_t arg[])
{
	char *buf = (char *)arg[0];
	size_t len = (size_t) arg[1];
	return sysfile_getcwd(buf, len);
}

static int sys_getdirentry(uint32_t arg[])
{
	int fd = (int)arg[0];
	struct dirent *direntp = (struct dirent *)arg[1];
	return sysfile_getdirentry(fd, direntp, NULL);
}

static int sys_dup(uint32_t arg[])
{
	int fd1 = (int)arg[0];
	int fd2 = (int)arg[1];
	return sysfile_dup(fd1, fd2);
}

static uint32_t sys_shmem(uint32_t arg[])
{
	uintptr_t *addr_store = (uintptr_t *) arg[0];
	size_t len = (size_t) arg[1];
	uint32_t mmap_flags = (uint32_t) arg[2];
	return do_shmem(addr_store, len, mmap_flags);
}

static uint32_t sys_mkdir(uint32_t arg[])
{
	const char *path = (const char *)arg[0];
	return sysfile_mkdir(path);
}

static uint32_t sys_unlink(uint32_t arg[])
{
	const char *name = (const char *)arg[0];
	return sysfile_unlink(name);
}


static uint32_t (*linux_syscall_table[1000]) (uint32_t arg[]) = {
  [__NR_exit] syscall_linux_exit,
  [__NR_wait4] syscall_linux_wait4,
  [__NR_fork] sys_fork,
  [__NR_read] syscall_linux_read,
  [__NR_write] syscall_linux_write,
  [__NR_open] syscall_linux_open,
	[__NR_access] syscall_linux_access,
  [__NR_close] syscall_linux_close,
  [__NR_execve] sys_exec,
  [__NR_chdir] syscall_linux_chdir,
  [__NR_time] syscall_linux_time,
  [__NR_lseek] syscall_linux_seek,
  [__NR_getpid] syscall_linux_getpid,
  [__NR_mount] syscall_linux_mount,
  [__NR_umount] syscall_linux_umount,
  [__NR_setuid] syscall_linux_setuid,
  [__NR_getuid] syscall_linux_getuid,
  [__NR_mkdir] syscall_linux_mkdir,
  [__NR_dup] syscall_linux_dup,
  [__NR_pipe] syscall_linux_pipe,
  [__NR_times] syscall_linux_times,
  [__NR_brk] syscall_linux_brk,
  [__NR_getuid] syscall_linux_getuid,
  [__NR_ioctl] syscall_linux_ioctl,
  [__NR_fcntl] syscall_linux_fcntl,
  [__NR_setrlimit] syscall_linux_setrlimit,
  [__NR_getrlimit] syscall_linux_getrlimit,
  [__NR_gettimeofday] syscall_linux_gettimeofday,
  [__NR_mmap] syscall_linux_mmap,
  [__NR_munmap] syscall_linux_munmap,
  [__NR_stat] syscall_linux_stat,
  [__NR_lstat] syscall_linux_lstat,
  [__NR_fstat] syscall_linux_fstat,
	[__NR_readlink] syscall_linux_readlink,
  [__NR_mprotect] syscall_linux_mprotect,
  [__NR_getdents] syscall_linux_getdents,
  [__NR_getdents64] syscall_linux_getdents64,
  [__NR_stat64] syscall_linux_stat64,
  [__NR_lstat64] syscall_linux_lstat64,
  [__NR_fstat64] syscall_linux_fstat64,
  [__NR_rt_sigaction] syscall_linux_sigaction,
  [__NR_rt_sigreturn] syscall_linux_sigreturn,
  [__NR_rt_sigprocmask] syscall_linux_sigprocmask,
  [__NR_readv] syscall_linux_readv,
  [__NR_writev] syscall_linux_writev,
  [__NR_getppid] syscall_linux_getppid,
  [__NR_getpgrp] syscall_linux_getpgrp,
  [__NR_getcwd] syscall_linux_getcwd,
  [__NR_geteuid] syscall_linux_geteuid,
  [__NR_getegid] syscall_linux_getegid,
  [__NR_getgid] syscall_linux_getgid,
  [__NR_setgid] syscall_linux_setgid,
  [__NR_setgroups] syscall_linux_setgroups,
  [__NR__newselect] syscall_linux_select,
  [__NR_poll] syscall_linux_poll,
  [__NR_dup2] syscall_linux_dup2,
  [__NR_uname] syscall_linux_uname,
  [__NR_socket] syscall_linux_socket,
  [__NR_connect] syscall_linux_connect,
  [__NR_accept] syscall_linux_accept,
  [__NR_sendto] syscall_linux_sendto,
  [__NR_recvfrom] syscall_linux_recvfrom,
  [__NR_send] syscall_linux_send,
  [__NR_recv] syscall_linux_recv,
  [__NR_bind] syscall_linux_bind,
  [__NR_listen] syscall_linux_listen,
  [__NR_getsockname] syscall_linux_getsockname,
  [__NR_getpeername] syscall_linux_getpeername,
  [__NR_setsockopt] syscall_linux_setsockopt,
  [__NR_getsockopt] syscall_linux_getsockopt,
  [__NR_setsid] syscall_linux_setsid,
  [__NR_nanosleep] syscall_linux_nanosleep,
  [__NR_ptrace] syscall_linux_ptrace,
};

void syscall_linux(void) {
  assert(current != NULL);
  struct trapframe *tf = current->tf;
  uint32_t arg[6];
  int num = tf->tf_regs.reg_r[MIPS_REG_V0] - 4000;
  if (num >= 0 && num < 1000) {
    if (linux_syscall_table[num] != NULL) {
      arg[0] = tf->tf_regs.reg_r[MIPS_REG_A0];
      arg[1] = tf->tf_regs.reg_r[MIPS_REG_A1];
      arg[2] = tf->tf_regs.reg_r[MIPS_REG_A2];
      arg[3] = tf->tf_regs.reg_r[MIPS_REG_A3];
      uint32_t* stack_pointor = tf->tf_regs.reg_r[MIPS_REG_SP];
      arg[4] = stack_pointor[4];
      arg[5] = stack_pointor[5];
      //kprintf("MIPS_sys $%d $%d\n", current->pid, num);
      //TODO: 0 indicates that syscall always suceeded, not reasonable but seems
      //It works. Need to be set before syscall because fork won't return.
      if(num == __NR_pipe) {
        int fd[2];
        if(file_pipe(fd) != 0) {
          tf->tf_regs.reg_r[MIPS_REG_A3] = 1;
        }
        else {
          tf->tf_regs.reg_r[MIPS_REG_A3] = 0;
          tf->tf_regs.reg_r[MIPS_REG_V0] = fd[0];
          tf->tf_regs.reg_r[MIPS_REG_V1] = fd[1];
          kprintf("pipe %d %d\n",  fd[0],  fd[1]);
        }
      }
      else {
        tf->tf_regs.reg_r[MIPS_REG_A3] = 0;
        tf->tf_regs.reg_r[MIPS_REG_V0] = linux_syscall_table[num] (arg);
      }
      /*if(num == __NR_read || num == __NR_write) {
        if(((int)tf->tf_regs.reg_r[MIPS_REG_V0]) < 0) {
          tf->tf_regs.reg_r[MIPS_REG_A3] = 1;
        }
      }*/
      return;
    }
  }
  print_trapframe(tf);
  panic("Undefined Linux OABI syscall %d, pid = %d, name = %s.\n",
      num, current->pid, current->name);
}

static int (*syscalls[]) (uint32_t arg[]) = {
[SYS_exit] sys_exit,
	    [SYS_fork] sys_fork,
	    [SYS_wait] sys_wait,
	    [SYS_exec] sys_exec,
	    [SYS_yield] sys_yield,
	    [SYS_kill] sys_kill,
	    [SYS_getpid] sys_getpid,
	    [SYS_brk] sys_brk,
	    [SYS_putc] sys_putc,
	    [SYS_pgdir] sys_pgdir,
	    [SYS_gettime] sys_gettime,
	    [SYS_sleep] sys_sleep,
	    [SYS_open] sys_open,
	    [SYS_close] sys_close,
	    [SYS_read] sys_read,
	    [SYS_write] sys_write,
	    [SYS_seek] sys_seek,
	    [SYS_fstat] sys_fstat,
	    [SYS_fsync] sys_fsync,
	    [SYS_chdir] sys_chdir,
	    [SYS_getcwd] sys_getcwd,
	    [SYS_getdirentry] sys_getdirentry,
	    [SYS_dup] sys_dup,
      [SYS_shmem] sys_shmem,
      [SYS_mount] syscall_linux_mount,
      [SYS_umount] syscall_linux_umount,
      [SYS_mkdir] sys_mkdir,
      [SYS_unlink] sys_unlink,
};

static int (*linux_syscalls[]) (uint32_t arg[]) = {
};

#define NUM_SYSCALLS        ((sizeof(syscalls)) / (sizeof(syscalls[0])))

void syscall(void)
{
	assert(current != NULL);
	struct trapframe *tf = current->tf;
	uint32_t arg[5];
	int num = tf->tf_regs.reg_r[MIPS_REG_V0];
  if(num >= 4000 && num <= 4999) {
    syscall_linux();
    return;
  }
	//num -= T_SYSCALL;
	//kprintf("$ %d %d\n",current->pid, num);
	if (num >= 0 && num < NUM_SYSCALLS) {
		if (syscalls[num] != NULL) {
			arg[0] = tf->tf_regs.reg_r[MIPS_REG_A0];
			arg[1] = tf->tf_regs.reg_r[MIPS_REG_A1];
			arg[2] = tf->tf_regs.reg_r[MIPS_REG_A2];
			arg[3] = tf->tf_regs.reg_r[MIPS_REG_A3];
			arg[4] = tf->tf_regs.reg_r[MIPS_REG_T0];
			tf->tf_regs.reg_r[MIPS_REG_V0] = syscalls[num] (arg);
			return;
		}
	}
	print_trapframe(tf);
	panic("undefined syscall %d, pid = %d, name = %s.\n",
	      num, current->pid, current->name);
}
