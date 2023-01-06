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

#define PLAY_DIR "/home/ubuntu/src/sandboxed_app/play_dir"

void setup_mounts(void);

void setup_sandbox(void) {
	printf("%s", "\n============== SETTING UP SANDBOX ===============\n");
	create_dir_if_not_exists(PLAY_DIR);
	// needed this to get permission to write to /proc/self/uid_map
	// https://stackoverflow.com/questions/47296408/
	prctl(PR_SET_DUMPABLE, 1, 0, 0, 0);

	// setup uid and gid mapping so that the uid inside user namespace is 1
	uid_t my_euid = geteuid();
	uid_t my_egid = getegid();
	if (0 != unshare(CLONE_NEWUSER | CLONE_NEWNS | CLONE_NEWPID)) {
		printf("%s\n", "Error while unsharing into new user+mount namespace");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
	char buffer[16];

	sprintf(buffer, "%u %u %u", 0, my_euid, 1);
	write_to_file("/proc/self/uid_map", buffer);

	// need to do this in order to be able to write to gid_map
	write_to_file("/proc/self/setgroups", "deny");

	sprintf(buffer, "%u %u %u", 0, my_egid, 1);
	write_to_file("/proc/self/gid_map", buffer);
	
	setup_mounts();

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
		puts("Inside child process\n");
		printf("%s\n", "printing dir /");
		print_dir("/");
		create_dir_if_not_exists("/proc");
		int ret = mount("proc", "/proc", "proc", 0, NULL);
		if (ret != 0) {
			printf("%s\n", "Error while creating /proc mount");
			perror("The following error occurred");
			exit(EXIT_FAILURE);
		}
	} else {
		// parent process
		puts("Inside parent process\n");
		int status = -1;
		waitpid(res_pid, &status, 0);
		puts("Exiting parent process\n");
		if (WIFEXITED(status)) {
			exit(WEXITSTATUS(status));
		} else {
			puts("Child process didnt exit normally\n");
			exit(EXIT_FAILURE);
		}
	}


}

void setup_mounts(void) {
	//make your mount points private so that outside world cant see them
	int ret = mount(NULL, "/", NULL, MS_PRIVATE | MS_REC , NULL);
	if (ret != 0) {
		printf("%s\n", "Error while making mounts private");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
	
	// whatever was in /tmp is still there
	// but have mounted a new tmpfs on top of it
	ret = mount("tmpfs", "/tmp", "tmpfs", 0, NULL);
	if (ret != 0) {
		printf("%s\n", "Error while creating /tmp mount");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
	// if want to share dir from outside into sandbox
	char common_dir[] = "/tmp/my_play_dir";
	create_dir_if_not_exists(common_dir);
	ret = mount(PLAY_DIR, common_dir, NULL, MS_BIND | MS_REC, NULL);
	if (ret != 0) {
		printf("Error while creating mount for %s\n", common_dir);
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}

	// could use chroot here- but cant join any further user namespaces if use that
	// this allows further namespaces, so use this

	char old_root[] = "/tmp/my_old_root";
	create_dir_if_not_exists(old_root);
	ret = syscall(SYS_pivot_root, "/tmp", old_root);
	
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

