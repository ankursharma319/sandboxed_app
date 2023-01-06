#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

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

