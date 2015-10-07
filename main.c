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
#define SEQUENTIAL 0 
#define PARALLEL 1

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

void create_process(char **one_command) {
	pid_t tpid;
	int child_pid = fork();
	int child_status;
	if (child_pid < 0) {
		fprintf(stderr, "fork failed, could not create child process\n");
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

void start_prompt() {
	char buffer[1024];
	int ex = 0;
	int mode = SEQUENTIAL;
	int new_mode = mode;
	while (fgets(buffer, 1024, stdin) != NULL) {
		
		char *comment;
		if ((comment = strchr(buffer, '#')) != NULL) {
			*comment = '\0';
		}		
		
		int i = 0;
		char **commands = tokenify(buffer, ";");
		char **one_command;
		while (commands[i] != NULL) {
			one_command = tokenify(commands[i], " \n\t");
			print_tokens(one_command);
			if ((strcmp(one_command[0], "exit")) == 0) {
				ex = 1;
				i++;
				continue;
			}
			if ((strcmp(one_command[0], "mode")) == 0) {
				if (one_command[1] != NULL) {
					if ((strcmp(one_command[1], "sequential")) == 0 ||
								(strcmp(one_command[1], "s")) == 0) {
						new_mode = SEQUENTIAL;
					}
					else if ((strcmp(one_command[1], "parallel")) == 0 ||
								(strcmp(one_command[1], "p")) == 0) {
						new_mode = PARALLEL;
					}
					else {
						printf("Invalid mode, please try again.\n");
					}
				}
				else {
					printf("Current mode: %d\n", mode);
				}
				printf("Mode is: %d\n", mode);
				i++;
				continue;
			}
			//create_process(one_command);
			
			pid_t tpid;
			int child_pid = fork();
			int child_status;
			if (child_pid < 0) {
				fprintf(stderr, "fork failed, could not create child process\n");
			} 
			else if (child_pid == 0) {
			// child: execv this command line
				if (execv(one_command[0],one_command) < 0) {
					fprintf(stderr, "execv failed: %s\n", strerror(errno));
				}
			}
			else if ((child_pid > 0) && mode == SEQUENTIAL) {
				// parent waits until child terminated
				printf("not in here");
				do {
					tpid = wait(&child_status);
					if (tpid == child_pid) { // if child terminated
						free_tokens(one_command);
						break;
					}
				} while(tpid != child_pid);
			}
			i++;
		}
		while (wait(NULL) > 0);
		// exit after finishing all commands
		if (ex == 1) {
			exit(1);
		}
		if (new_mode != mode) {
			mode = new_mode;
		}
		free_tokens(commands);
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
