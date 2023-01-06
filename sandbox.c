#include "sandbox.h"
#include "utils.h"

#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sched.h>

void setup_sandbox(void) {
	printf("%s", "\n============== SETTING UP SANDBOX ===============\n");
	// needed this to get permission to write to /proc/self/uid_map
	// https://stackoverflow.com/questions/47296408/
	prctl(PR_SET_DUMPABLE, 1, 0, 0, 0);

	// setup uid and gid mapping so that the uid inside user namespace is 1
	uid_t my_euid = geteuid();
	uid_t my_egid = getegid();
	if (0 != unshare(CLONE_NEWUSER)) {
		printf("%s\n", "Error while unsharing into new user namespace");
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

}
