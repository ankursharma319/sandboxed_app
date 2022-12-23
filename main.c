
#define _GNU_SOURCE     /* To get defns of NI_MAXSERV and NI_MAXHOST */

#include <sys/capability.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

// for network interface
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>

void print_ids(void);
void print_root_dirs(void);
void print_capabilities(void);
char* file_entry_type_to_str(unsigned char type);
void print_pids(void);
void print_network_interfaces(void);

int main(void) {
	printf("%s\n", "Running application");
	print_ids();
	print_root_dirs();
	print_capabilities();
	print_pids();
	print_network_interfaces();
	return 0;
}

void print_ids(void) {
	printf("\n============== IDs ===============\n");
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
	printf("\n============== Root dir (/) ===============\n");
	errno = 0;
	DIR* root_dir = opendir("/");
	if (!root_dir) {
		printf("%s\n", "Error while opening root dir");
		perror("The following error occurred");
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

void print_capabilities(void) {
	printf("\n============== Capabilities ===============\n");
	cap_t ret = cap_get_proc();
	if (ret == NULL) {
		printf("%s\n", "Error while getting capabilities");
		perror("The following error occurred");
		return;
	}
	char * txt_caps = cap_to_text(ret, NULL);
	if (txt_caps == NULL) {
		printf("%s\n", "Error while converting capabilities to txt");
	}
	printf("%s\n", txt_caps);
	if (cap_free(txt_caps) != 0 || cap_free(ret) != 0) {
		printf("%s\n", "Error while freeing capabilities mem");
	}
}

void print_pids(void) {
	printf("\n============== All PIDs ===============\n");
	errno = 0;
	DIR* root_dir = opendir("/proc/");
	if (!root_dir) {
		printf("%s\n", "Error while opening proc dir");
		perror("The following error occurred");
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
	printf("\n============== Network Interfaces ===============\n");
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

