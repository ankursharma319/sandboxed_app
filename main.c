
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

void print_ids(void);
void print_root_dirs(void);
char* file_entry_type_to_str(unsigned char type);

int main(void) {
  printf("%s\n", "Running application");
  print_ids();
  print_root_dirs();
  return 0;
}

void print_ids(void) {

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

	errno = 0;
	DIR* root_dir = opendir("/");
	if (!root_dir) {
		printf("%s\n", "Error while opening root dir");
		perror("The following error occurred");
	}

	printf("%s\n", "Root dir:");
	errno = 0;
	struct dirent * entry; 
	while ((entry = readdir(root_dir)) != NULL) {
    	printf("  %s (%s)\n", entry->d_name, file_entry_type_to_str(entry->d_type));
	}
    closedir(root_dir);
	if (errno != 0) {
		printf("%s\n", "Error while reading root dir");
		perror("The following error occurred");
	}
}

