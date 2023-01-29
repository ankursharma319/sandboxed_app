
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/capability.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/resource.h>
#include <string.h>

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

void raise_capabilities(void);
void drop_capabilities(void);
void edit_files(void);
void allocate_large_mem(void);

int main(void) {
	printf("%s\n", "Running application");
	// need to run as root in order to move process from
	// root owned cgroup to a new cgroup
	// but only need to access to write to cgroup dir owned
	// by root and should not need any other capabilities
	drop_capabilities();
	print_all();
	allocate_large_mem();
	setup_sandbox();
	print_all();
	edit_files();
	printf("\nThe following should fail because of cgroup memory limit\n");
	allocate_large_mem();
	return 0;
}

void print_all(void) {
	print_ids();
	print_root_dirs();
	print_capabilities();
	print_pids();
	print_network_interfaces();
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

void drop_capabilities(void) {
	printf("%s", "Dropping all capabilities\n");

	// need setfcap to create directory
	// need dac_override to be able to write to files not owned by us, specifically cgroup.procs
	// cap_t caps = cap_from_text("= all+eip");
	cap_t caps = cap_from_text("= cap_dac_override,cap_setfcap+eip");
	if (caps == NULL) {
		/* handle error */;
		printf("%s\n", "Error while getting capabilities");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}

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

void raise_capabilities(void) {
	// Raising some capabilities in the effective set
	// Needs to be already present in the permitted set
	// which can be done using the setcap(8) cmd on the executable
	// e.g. sudo setcap all=p ./_build/release/sandboxed_executable

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

void edit_files(void) {
	printf("%s", "\n============== /oldroot ===============\n");
	print_dir("/oldroot");
	printf("%s", "\n============== /my_play_dir ===============\n");
	print_dir("/my_play_dir");
	printf("%s", "\n============== Writing current timestamp to files ===============\n");
	char buffer[32];
	get_current_time_str(buffer, sizeof(buffer));
	// printf("got time_str %s\n", buffer);
	write_to_file("/my_play_dir/my_file.txt", buffer); 
	write_to_file("/oldroot/tmp/my_file.txt", buffer); 
	write_to_file("/my_file.txt", buffer);
	print_file_contents("/my_play_dir/my_file.txt");
	print_file_contents("/oldroot/tmp/my_file.txt");
	print_file_contents("/my_file.txt");
	// outside the process, will see /tmp/my_file.txt and /tmp/my_play_dir/my_file.txt
	// but not /tmp/sandbox_tmp/my_file.txt
	// confirm that manually
}

void allocate_large_mem(void) {
	printf("%s", "\n============== Memory test ===============\n");
	printf("%s\n", "Attempting to allocate large amounts of memory");
    size_t n_blocks = 4;
    size_t block_size = 1024*1024*4;
    printf("I will eat memory in %zu blocks of size %zu\n", n_blocks, block_size);
    printf("Press any key to continue\n");
    getchar();
    uint8_t* buf[n_blocks];
    for (size_t i=0; i<n_blocks; i++) {
        buf[i] = (uint8_t*) calloc(block_size, 1);
        if (buf[i] == NULL) {
            printf("%s\n", "Error while calling calloc");
            perror("calloc");
            exit(EXIT_FAILURE);
        }
        printf("allocated block# i=%zu, filling it with rand data now\n", i);
        for (size_t j=0; j<block_size; j++) {
            buf[i][j] = rand() * UINT8_MAX;
        }
    }
    printf("Press any key to continue to the end\n");
    getchar();
    for (size_t i=0; i<n_blocks; i++) {
        uint8_t sum = 0;
        for (size_t j=0; j<block_size; j++) {
            sum += buf[i][j];
        }
        printf("sum at index i=%zu is %hhu\n", i, sum);
        free(buf[i]);
    }
    puts("releasing allocated memory\n");
}

