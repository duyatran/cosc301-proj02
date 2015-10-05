#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <signal.h>
#define PROMPT "prompt > "

void check_input(char **str) {
    char *comment;
    if ((comment = strchr(str, '#')) != NULL) {
		*comment = '\0';
	}
}

char** tokenify(const char *s) {
    char *copy1 = strdup(s);
    char *saveptr1;
	char *count = strtok_r(copy1, " \n\t", &saveptr1);
    int len = 0;
    while (count != NULL) {
		len++;
		count = strtok_r(NULL, " \n\t", &saveptr1);
	}
	free(copy1);
	
	char **tokens = malloc(sizeof(char *) * (len+1));
    char *copy2 = strdup(s);
    char *saveptr2;
    char *token = strtok_r(copy2, " \n\t", &saveptr2);
    int i = 0;
    for (i = 0; token != NULL; i++) {
		tokens[i] = strdup(token);
		token = strtok_r(NULL, " \n\t", &saveptr2);
	}
	free(copy2);
	tokens[i] = NULL;
	return tokens;
}

void free_tokens(char **tokens) {
    int i = 0;
    while (tokens[i] != NULL) {
        free(tokens[i]); // free each string
        i++;
    }
    free(tokens); // then free the array
}

void create_process(char ***proc) {
    pid_t tpid;
	int child_pid = fork();
	int child_status;
    if (child_pid < 0) {
        // fork failed;
        fprintf(stderr, "fork failed\n");
        //exit(1);
    } 
    
    else if (child_pid == 0) {
	// child: execv this command line
		if (execv(*proc[0],*proc) < 0) {
			fprintf(stderr, "execv failed: %s\n", strerror(errno));
		}
    } 
    
    else {
        // parent waits until child terminated
		do {
			tpid = wait(&child_status);
			if (tpid == child_pid) {
				//do something, probably break
				break;
			}
     } while(tpid != child_pid);
    }
}

void print_tokens(char *tokens[]) {
    int i = 0;
    while (tokens[i] != NULL) {
        printf("Token %d: %s\n", i+1, tokens[i]);
        i++;
    }
}

void start_prompt() {
	char buffer[1024];
	char spaces[] = " \n\t";
	char end_command = ";";
	while (fgets(buffer, 1024, stdin) != NULL) {
		check_input(&buffer);
		
		char **args = tokenify(buffer);
		create_process(&args);
		free_tokens(args);
		
		printf("%s", PROMPT);
		fflush(stdout);
	}
}

int main(int argc, char **argv) {
	printf("%s", PROMPT);
	fflush(stdout);
	start_prompt();
    return 0;
}
