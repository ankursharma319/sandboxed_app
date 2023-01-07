#include "utils.h"

#include <ctype.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>

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

void print_file_contents(char const * const filename) {
	FILE* fp = fopen(filename, "r");
	if(!fp) {
		printf("Error while opening %s\n", filename);
		perror("The following error occurred");
        exit(EXIT_FAILURE);
    }
	printf("Printing file contents of %s:\n", filename);
	char c = fgetc(fp);
    while (c != EOF)
    {
        printf ("%c", c);
        c = fgetc(fp);
    }
	printf("\n");
	fclose(fp);
}

bool dir_exists(char const * const dirname) {
	return access(dirname, F_OK) >= 0;
}

void create_dir_if_not_exists(char const * const dirname) {
	if (!dir_exists(dirname)) {
		if (mkdir(dirname, 0777) < 0) {
			printf("Error while creating dir %s\n", dirname);
			perror("The following error occurred");
			exit(EXIT_FAILURE);
		}
	}
}

void print_dir(char const* const dirname) {
	errno = 0;
	DIR* root_dir = opendir(dirname);
	if (!root_dir) {
		printf("Error while opening dir %s\n", dirname);
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}

	errno = 0;
	struct dirent * entry; 
	while ((entry = readdir(root_dir)) != NULL) {
		printf("  %s (%s)\n", entry->d_name, file_entry_type_to_str(entry->d_type));
	}
	if (errno != 0) {
		printf("Error while reading dir %s\n", dirname);
		perror("The following error occurred");
	}
	closedir(root_dir);
}

void print_dir_nums(char const* const dirname) {
	errno = 0;
	DIR* root_dir = opendir(dirname);
	if (!root_dir) {
		printf("Error while opening dir %s\n", dirname);
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}

	errno = 0;
	struct dirent * entry; 
	while ((entry = readdir(root_dir)) != NULL) {
		if (isdigit(entry->d_name[0])) {
			printf("  %s (%s)\n", entry->d_name, file_entry_type_to_str(entry->d_type));
		}
	}
	if (errno != 0) {
		printf("Error while reading dir %s\n", dirname);
		perror("The following error occurred");
	}
	closedir(root_dir);
}

void get_current_time_str(char * const string, size_t max_size) {
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	if (!tm) {
		printf("Error calling localtime\n");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
	size_t ret = strftime(string, max_size, "%c", tm);
	if (!ret) {
		printf("Error calling strftime\n");
		perror("The following error occurred");
		exit(EXIT_FAILURE);
	}
}

