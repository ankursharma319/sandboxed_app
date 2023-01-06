
#define _GNU_SOURCE     /* To get defns of NI_MAXSERV and NI_MAXHOST */

#include <sys/capability.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sched.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/prctl.h>

// for network interface
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>

void write_to_file(char const * const filename, char const * const text);

void print_all(void);
void print_ids(void);
void print_root_dirs(void);
void print_capabilities(void);
char* file_entry_type_to_str(unsigned char type);
void print_pids(void);
void print_network_interfaces(void);
void print_nice_values(void);

void raise_capabilities(void);
void raise_nice(void);
void setup_sandbox(void);

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

char* file_entry_type_to_str(unsigned char type) {
	if (type == DT_BLK) {
		return "block";
	}
	if (type == DT_CHR) {
		return "char";
	}
	if (type == DT_DIR) {
		return "dir";
	}
	if (type == DT_FIFO) {
		return "fifo";
	}
	if (type == DT_LNK) {
		return "link";
	}
	if (type == DT_REG) {
		return "file";
	}
	if (type == DT_SOCK) {
		return "socket";
	}
	return "unknown";
}

void print_root_dirs(void) {
	printf("%s", "\n============== Root dir (/) ===============\n");
	errno = 0;
	DIR* root_dir = opendir("/");
	if (!root_dir) {
		printf("%s\n", "Error while opening root dir");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}

	errno = 0;
	struct dirent * entry; 
	while ((entry = readdir(root_dir)) != NULL) {
		printf("  %s (%s)\n", entry->d_name, file_entry_type_to_str(entry->d_type));
	}
	if (errno != 0) {
		printf("%s\n", "Error while reading root dir");
		perror("The following error occurred");
	}
	closedir(root_dir);
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
	errno = 0;
	DIR* root_dir = opendir("/proc/");
	if (!root_dir) {
		printf("%s\n", "Error while opening proc dir");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}

	errno = 0;
	struct dirent * entry; 
	while ((entry = readdir(root_dir)) != NULL) {
		printf("  %s (%s)\n", entry->d_name, file_entry_type_to_str(entry->d_type));
	}
	if (errno != 0) {
		printf("%s\n", "Error while reading proc dir");
		perror("The following error occurred");
	}
	closedir(root_dir);
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
	printf("%s\n", "Raising nice (priority) values");
	int process_prio = getpriority(PRIO_PROCESS, 0);
	if (errno != 0) {
		printf("%s\n", "Error while retrieving process priority value");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
	int ret = setpriority(PRIO_PROCESS, 0, process_prio-1);
	if (ret != 0) {
		printf("%s\n", "Error while setting process priority value");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
}

void write_to_file(char const * const filename, char const * const text) {
	FILE* fp = fopen(filename, "w");
	if(!fp) {
		printf("Error while opening %s\n", filename);
		perror("The following error occurred");
        exit(EXIT_FAILURE);
    }
	int ret = fprintf(fp, "%s", text);
	if(ret <= 0) {
		printf("Error while writing to %s\n", filename);
		perror("The following error occurred");
        exit(EXIT_FAILURE);
	}
	fclose(fp);
}

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

