#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>


void load_directories(const char * filename) {
	FILE *file = fopen(filename, "r");
	char line[32];
	while (fgets(line, 32, file)) {
		printf("%s\n",line);
	}
	
	fclose(file);
}

int main(int argc, char **argv) {
	load_directories("./shell-config");
    return 0;
}
