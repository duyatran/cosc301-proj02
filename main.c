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

void print_tokens(char *tokens[]) {
    int i = 0;
    while (tokens[i] != NULL) {
        printf("Token %d: %s\n", i+1, tokens[i]);
        i++;
    }
}

char** tokenify(const char *s, const char *delimit) {
    char *copy1 = strdup(s);
    char *saveptr1;
	char *count = strtok_r(copy1, delimit, &saveptr1);
    int len = 0;
    while (count != NULL) {
		len++;
		count = strtok_r(NULL, delimit, &saveptr1);
	}
	free(copy1);
	
	char **tokens = malloc(sizeof(char *) * (len+1));
    char *copy2 = strdup(s);
    char *saveptr2;
    char *token = strtok_r(copy2, delimit, &saveptr2);
    int i = 0;
    for (i = 0; token != NULL; i++) {
		tokens[i] = strdup(token);
		token = strtok_r(NULL, delimit, &saveptr2);
	}
	free(copy2);
	tokens[i] = NULL;
	return tokens;
}

void free_tokens(char **tokens) {
    int i = 0;
    while (tokens[i] != NULL) {
        free(tokens[i]); // free each token
        i++;
    }
    free(tokens); // then free the array of tokens
}

void create_process(char **commands, int *exit_code) {
	int mode = 0; // 0 is sequential, 1 is parallel
	int special= 0;
	char **one_command = tokenify(*commands, " \n\t");
	if ((strcmp(one_command[0], "exit")) == 0) {
		special = 1;
		*exit_code = 1;
	}
	//else if (strcmp(one_command[0], "mode") {
		
	//}
	if (special == 0) {
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
			if (execv(one_command[0],one_command) < 0) {
				fprintf(stderr, "execv failed: %s\n", strerror(errno));
			}
		} 
		else {
			// parent waits until child terminated
			do {
				tpid = wait(&child_status);
				if (tpid == child_pid) { // if child terminated
					free_tokens(one_command);
					break;
				}
			} while(tpid != child_pid);
		}
	}
}

void start_prompt() {
	char buffer[1024];
	int ex = 0;
	while (fgets(buffer, 1024, stdin) != NULL) {
		check_input(&buffer);
		char **commands = tokenify(buffer, ";");
		printf("list of commands\n");
		print_tokens(commands);
		printf("end\n");
		int i = 0;
		while (commands[i] != NULL) {
			create_process(&(commands[i]), &ex);
			i++;
		}
		free_tokens(commands);
		if (ex == 1) {
			exit(1);
		}
		printf("%s", PROMPT);
		fflush(stdout);
	}
	printf("\n"); // EOF reached, just print new line before exiting
}

int main(int argc, char **argv) {
	printf("%s", PROMPT);
	fflush(stdout);
	start_prompt();
    return 0;
}
