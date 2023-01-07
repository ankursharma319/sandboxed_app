#include "sandbox.h"
#include "utils.h"

#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sched.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <seccomp.h>

#define PLAY_DIR "/tmp/sandbox_play_dir"

void setup_namespaces(void);
void setup_mounts(void);
void setup_seccomp(void);
void allow(scmp_filter_ctx ctx, int syscall);

void setup_sandbox(void) {
	printf("%s", "\n============== SETTING UP SANDBOX ===============\n");
	setup_namespaces();

	// mount /proc
	// make sure that we are in a new pid namespace bcoz
	// the root pid namespace already has /proc mounted
	// need to move the current process into the new pid namespace manually
	// unshare creates the pid namespace without automatically moving current
	// process into that namespaces
	// the first child of the current process (after unshare has been called)
	// will be pid 1 in the new namespace

	pid_t res_pid = fork();
	if (res_pid == -1) {
		printf("%s\n", "Error while forking");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
	if (res_pid == 0) {
		// child process
		puts("Inside child process");
		create_dir_if_not_exists("/proc");
		int ret = mount("proc", "/proc", "proc", 0, NULL);
		if (ret != 0) {
			printf("%s\n", "Error while creating /proc mount");
			perror("The following error occurred");
			exit(EXIT_FAILURE);
		}
		// the reason for moving the child process only to new network namespace
		// is that it can allow us access to the sandbox net ns in the child process
		// and allow access to the root net ns in the parent process and allow to
		// setup bridge between them or iptables magic
		if (0 != unshare(CLONE_NEWNET)) {
			printf("%s\n", "Error while unsharing into new network namespace");
			perror("The following error occurred");
			exit(EXIT_FAILURE);
		}
		setup_seccomp();
	} else {
		// parent process
		puts("Inside parent process");
		int status = -1;
		waitpid(res_pid, &status, 0);
		puts("Exiting parent process");
		if (WIFEXITED(status)) {
			exit(WEXITSTATUS(status));
		} else {
			puts("Child process didnt exit normally");
			exit(EXIT_FAILURE);
		}
	}


}

void setup_namespaces(void) {
	create_dir_if_not_exists(PLAY_DIR);
	// needed this to get permission to write to /proc/self/uid_map
	// https://stackoverflow.com/questions/47296408/
	prctl(PR_SET_DUMPABLE, 1, 0, 0, 0);

	// setup uid and gid mapping so that the uid inside user namespace is 1
	uid_t my_euid = geteuid();
	uid_t my_egid = getegid();
	printf("%s\n", "Entering into new user+mount+pid namespace");
	if (0 != unshare(CLONE_NEWUSER | CLONE_NEWNS | CLONE_NEWPID)) {
		printf("%s\n", "Error while unsharing into new user+mount+pid namespace");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
	char buffer[16];

	sprintf(buffer, "%u %u %u", 0, my_euid, 1);
	printf("writing following to uid_map : %s\n", buffer);
	write_to_file("/proc/self/uid_map", buffer);
	print_file_contents("/proc/self/uid_map");
	// need to do this in order to be able to write to gid_map
	write_to_file("/proc/self/setgroups", "deny");

	sprintf(buffer, "%u %u %u", 0, my_egid, 1);
	printf("writing following to gid_map : %s\n", buffer);
	write_to_file("/proc/self/gid_map", buffer);
	print_file_contents("/proc/self/gid_map");
	
	setup_mounts();
}

void setup_mounts(void) {
	//make your mount points private so that outside world cant see them
	int ret = mount(NULL, "/", NULL, MS_PRIVATE | MS_REC , NULL);
	if (ret != 0) {
		printf("%s\n", "Error while making mounts private");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}

	char tmp_dir[] = "/tmp/sandbox_tmp";
	create_dir_if_not_exists(tmp_dir);
	// whatever was in /tmp is still there
	// but have mounted a new tmpfs on top of it
	ret = mount("tmpfs", tmp_dir, "tmpfs", 0, NULL);
	if (ret != 0) {
		printf("%s\n", "Error while creating tmp_dir mount");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
	// if want to share dir from outside into sandbox
	char common_dir[] = "/tmp/sandbox_tmp/my_play_dir";
	create_dir_if_not_exists(common_dir);
	ret = mount(PLAY_DIR, common_dir, NULL, MS_BIND | MS_REC, NULL);
	if (ret != 0) {
		printf("Error while creating mount for %s\n", common_dir);
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}

	// could use chroot here- but cant join any further user namespaces if use that
	// this allows further namespaces, so use this

	char old_root[] = "/tmp/sandbox_tmp/my_old_root";
	create_dir_if_not_exists(old_root);
	ret = syscall(SYS_pivot_root, tmp_dir, old_root);

	ret = chdir("/");
	if (ret != 0) {
		printf("%s\n", "Error while chdir to /");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}

	// if dont do this unmount
	// then /my_old_root will give the sandboxed app access to the outside world

	// char old_root_2[] = "/my_old_root";
	// ret = umount2(old_root_2, MNT_DETACH);
	// if (ret != 0) {
	// 	printf("Error while unmounting %s\n", old_root_2);
	// 	perror("The following error occurred");
	// 	exit(EXIT_FAILURE);
	// }
	// ret = rmdir(old_root_2);
	// if (ret != 0) {
	// 	printf("%s\n", "Error while rmdirring old_root");
	// 	perror("The following error occurred");
	// 	exit(EXIT_FAILURE);
	// }

}

void allow(scmp_filter_ctx ctx, int syscall) {
	int ret = seccomp_rule_add(ctx, SCMP_ACT_ALLOW, syscall, 0);
	if (ret != 0) {
		printf("Error while trying to allow syscall %d in seccomp\n", syscall);
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
}

void allow_cleanup(scmp_filter_ctx ctx)
{
    allow(ctx, SCMP_SYS(sigreturn));
    allow(ctx, SCMP_SYS(rt_sigaction));
    allow(ctx, SCMP_SYS(rt_sigreturn));
    allow(ctx, SCMP_SYS(exit));
    allow(ctx, SCMP_SYS(exit_group));
    allow(ctx, SCMP_SYS(kill));
}

void allow_polling(scmp_filter_ctx ctx)
{
    allow(ctx, SCMP_SYS(poll));
    allow(ctx, SCMP_SYS(ppoll));
    allow(ctx, SCMP_SYS(epoll_wait));
    allow(ctx, SCMP_SYS(epoll_pwait));
    allow(ctx, SCMP_SYS(epoll_create1));
    allow(ctx, SCMP_SYS(epoll_ctl));
}

void allow_time_functions(scmp_filter_ctx ctx)
{
    allow(ctx, SCMP_SYS(clock_gettime));
    allow(ctx, SCMP_SYS(gettimeofday));
}

void allow_fd_access(scmp_filter_ctx ctx)
{
	allow(ctx, SCMP_SYS(open));
	allow(ctx, SCMP_SYS(openat));
    allow(ctx, SCMP_SYS(close));
    allow(ctx, SCMP_SYS(write));
    allow(ctx, SCMP_SYS(read));
    allow(ctx, SCMP_SYS(readv));
    allow(ctx, SCMP_SYS(fstat));
    allow(ctx, SCMP_SYS(newfstatat));
    allow(ctx, SCMP_SYS(getdents));
    allow(ctx, SCMP_SYS(getdents64));
    allow(ctx, SCMP_SYS(fstat64));
    allow(ctx, SCMP_SYS(ioctl));
    allow(ctx, SCMP_SYS(dup));
    allow(ctx, SCMP_SYS(dup2));
    allow(ctx, SCMP_SYS(fcntl));
}

void allow_events_and_timers(scmp_filter_ctx ctx)
{
    allow(ctx, SCMP_SYS(eventfd2));
    allow(ctx, SCMP_SYS(timerfd_settime));
    allow(ctx, SCMP_SYS(timerfd_create));
}

void allow_std_allocations(scmp_filter_ctx ctx)
{
    allow(ctx, SCMP_SYS(brk));
    allow(ctx, SCMP_SYS(mmap));
    allow(ctx, SCMP_SYS(mmap2));
    allow(ctx, SCMP_SYS(munmap));
}

void allow_sandboxing_related(scmp_filter_ctx ctx) {
	allow(ctx, SCMP_SYS(unshare));
	allow(ctx, SCMP_SYS(getuid));
	allow(ctx, SCMP_SYS(geteuid));
	allow(ctx, SCMP_SYS(getgid));
	allow(ctx, SCMP_SYS(getegid));
	allow(ctx, SCMP_SYS(getpid));
	allow(ctx, SCMP_SYS(getppid));
	allow(ctx, SCMP_SYS(capget));
	allow(ctx, SCMP_SYS(capset));
	allow(ctx, SCMP_SYS(getpriority));
	allow(ctx, SCMP_SYS(setpriority));
}

void allow_networking_related(scmp_filter_ctx ctx) {
	allow(ctx, SCMP_SYS(socket));
	allow(ctx, SCMP_SYS(bind));
	allow(ctx, SCMP_SYS(getsockname));
	allow(ctx, SCMP_SYS(sendto));
	allow(ctx, SCMP_SYS(recvmsg));
}

void setup_seccomp(void) {
	printf("%s\n", "Setting up seccomp");
	scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_KILL);
	if (ctx == NULL) {
		printf("%s\n", "Error in seccomp_init");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}

	allow_cleanup(ctx);
	allow_events_and_timers(ctx);
	allow_fd_access(ctx);
	allow_polling(ctx);
	allow_std_allocations(ctx);
	allow_time_functions(ctx);
	allow_sandboxing_related(ctx);
	allow_networking_related(ctx);

	int ret = seccomp_load(ctx);
	if (ret != 0) {
		printf("Error code %d while seccomp_load\n", ret);
		exit(EXIT_FAILURE);
	}
	seccomp_release(ctx);
}

