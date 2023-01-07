
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/capability.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/resource.h>

// for network interface
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <linux/if_link.h>

#include "utils.h"
#include "sandbox.h"

void print_all(void);
void print_ids(void);
void print_root_dirs(void);
void print_capabilities(void);
void print_pids(void);
void print_network_interfaces(void);
void print_nice_values(void);

void raise_capabilities(void);
void raise_nice(void);

int main(void) {
	printf("%s\n", "Running application");
	raise_capabilities();
	// raise_nice();
	print_all();
	setup_sandbox();
	print_all();
	return 0;
}

void print_all(void) {
	print_ids();
	print_root_dirs();
	print_capabilities();
	print_pids();
	print_network_interfaces();
	print_nice_values();
}

void print_ids(void) {
	printf("%s", "\n============== IDs ===============\n");
	uid_t my_ruid = getuid();
	uid_t my_euid = geteuid();
	uid_t my_rgid = getgid();
	uid_t my_egid = getegid();
	pid_t my_pid = getpid();
	pid_t my_ppid = getppid();
	printf("Real uid = %u\n", my_ruid);
	printf("Effective uid = %u\n", my_euid);
	printf("Real gid = %u\n", my_rgid);
	printf("Effective gid = %u\n", my_egid);
	printf("Process Id (PID) = %d\n", my_pid);
	printf("Parent Process Id (PPID) = %d\n", my_ppid);
}

void print_root_dirs(void) {
	printf("%s", "\n============== Root dir (/) ===============\n");
	print_dir("/");
}

void raise_capabilities(void) {
	// Raising some capabilities in the effective set
	// Needs to be already present in the permitted set
	// which can be done using the setcap(8) cmd on the executable

	printf("%s", "Raising some capabilities\n");

	// cap_t caps = cap_from_text("=p cap_fowner,cap_setfcap+e");
	cap_t caps = cap_from_text("=");
	if (caps == NULL) {
		/* handle error */;
		printf("%s\n", "Error while getting capabilities");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}

	/* cap_t caps;
	caps = cap_get_proc();
	if (caps == NULL) {
		// handle error
		printf("%s\n", "Error while getting capabilities");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
	cap_value_t cap_list[2];
	cap_list[0] = CAP_FOWNER;
	cap_list[1] = CAP_SETFCAP;
	if (cap_set_flag(caps, CAP_EFFECTIVE, 2, cap_list, CAP_SET) == -1) {
		printf("%s\n", "Error while setting capabilities");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	} */

	if (cap_set_proc(caps) == -1) {
		/* handle error */;
		printf("%s\n", "Error while setting capabilities");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
	if (cap_free(caps) == -1) {
		printf("%s\n", "Error while freeing capabilities mem");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
}

void print_capabilities(void) {
	printf("%s", "\n============== Capabilities ===============\n");
	cap_t ret = cap_get_proc();
	if (ret == NULL) {
		printf("%s\n", "Error while getting capabilities");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
	char * txt_caps = cap_to_text(ret, NULL);
	if (txt_caps == NULL) {
		printf("%s\n", "Error while converting capabilities to txt");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
	printf("%s\n", txt_caps);
	if (cap_free(txt_caps) != 0 || cap_free(ret) != 0) {
		printf("%s\n", "Error while freeing capabilities mem");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
}

void print_pids(void) {
	printf("%s", "\n============== All PIDs ===============\n");
	print_dir_nums("/proc");
}

void print_network_interfaces(void) {
	printf("%s", "\n============== Network Interfaces ===============\n");
	struct ifaddrs *ifaddr;
	int family, s;
	char host[NI_MAXHOST];

	if (getifaddrs(&ifaddr) == -1) {
		printf("%s\n", "Error while calling getifaddrs");
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}

	/* Walk through linked list, maintaining head pointer so we
	  can free list later. */

	for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL) {
			continue;
		}

		family = ifa->ifa_addr->sa_family;

		/* Display interface name and family (including symbolic
		  form of the latter for the common families). */

		printf("%-8s %s (%d)\n",
			  ifa->ifa_name,
			  (family == AF_PACKET) ? "AF_PACKET" :
			  (family == AF_INET) ? "AF_INET" :
			  (family == AF_INET6) ? "AF_INET6" : "???",
			  family);

		/* For an AF_INET* interface address, display the address. */

		if (family == AF_INET || family == AF_INET6) {
		   	s = getnameinfo(
				ifa->ifa_addr,
				(family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
				host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST
			);
			if (s != 0) {
				printf("getnameinfo() failed: %s\n", gai_strerror(s));
				exit(EXIT_FAILURE);
		   	}

		   	printf("\t\taddress: <%s>\n", host);

		} else if (family == AF_PACKET && ifa->ifa_data != NULL) {
			struct rtnl_link_stats *stats = ifa->ifa_data;

			printf(
				"\t\ttx_packets = %10u; rx_packets = %10u\n"
				"\t\ttx_bytes   = %10u; rx_bytes   = %10u\n",
				stats->tx_packets, stats->rx_packets,
				stats->tx_bytes, stats->rx_bytes
			);
		}
	}

	freeifaddrs(ifaddr);
}

void print_nice_values(void) {
	printf("%s", "\n============== Nice (Priority) values ===============\n");
	errno = 0;
	int process_prio = getpriority(PRIO_PROCESS, 0);
	int user_prio = getpriority(PRIO_USER, 0);
	if (errno != 0) {
		printf("%s\n", "Error while retrieving process priority value");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
	printf("Process priority = %d\n", process_prio);
	printf("User priority = %d\n", user_prio);
}

void raise_nice(void) {
	// example of something which needs CAP_SYS_NICE or root privilige
	// should fail outside of sandbox because we dont have the capability
	// and should fail inside the sandbox because this is a global resource,
	// does not fall under any namespace belonging to user namespace in sandbox
	printf("%s\n", "Raising nice (priority) values");
	errno = 0;
	int process_prio = getpriority(PRIO_PROCESS, 0);
	if (errno != 0) {
		printf("%s, process_prio = %d\n", "Error while retrieving process priority value", process_prio);
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
	errno = 0;
	int ret = setpriority(PRIO_PROCESS, 0, process_prio-1);
	if (ret != 0) {
		printf("%s\n", "Error while setting process priority value");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
}

