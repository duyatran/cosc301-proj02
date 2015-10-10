/*
 * DUY TRAN & HA VU
 */
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
#define RUNNING 1
#define PAUSED 0

char ** load_directories(const char * filename) {
	FILE *dir_file = fopen(filename, "r");
	char next[32];
	int len = 0;
    while (fgets(next, 32, dir_file) != NULL) {
		len++;
	}
	char** dir_list = malloc(sizeof(char *) * (len+1));
	fclose(dir_file);
	dir_file = fopen(filename, "r");
	int i = 0;
	while (fgets(next, 32, dir_file) != NULL){
		next[strlen(next)-1] = '\0';
		dir_list[i] = strdup(next);
		i++;
	}
	dir_list[i] = NULL;
	fclose(dir_file);
	return dir_list;
}

void free_array(char** arr) {
	int i=0;
	while(arr[i]!=NULL){
		free(arr[i]);
		i++;
	}
	free(arr[i]);
	free(arr);
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

typedef struct _process{
	pid_t pid;
	char ** command;
	int state;
	struct _process* next;
} process;

process *add_process(process *head, pid_t pid, 
						char **command, int state) {
	process *new_head = malloc(sizeof(process));
	new_head->command = command;
    new_head->pid = pid;
    new_head->state = state;
    new_head->next = head;
	return new_head;
}

process *delete_process(process *head, pid_t pid){
	process *tmp = head;	
	if (head->pid == pid){
		head = head->next;
	}
	else {
		while (tmp->next != NULL){
			process *before = tmp;
			tmp = tmp->next;
			if (tmp->pid == pid){
				before->next = tmp->next;
				break;
			}
		}
	}
	free_array(tmp->command);
	free(tmp);
	return head;
}

process *find_pid(process *head, pid_t pid){
	if (head != NULL && head->pid != pid) {
		process * tmp = head;
		while (tmp->next != NULL){
			tmp = tmp->next;
			if (tmp->pid==pid){
				return tmp;
			}
		}
		return NULL;
	}
	return head;
}

void print_process(process *head){
	if (head == NULL){
		printf("There are no processes running.\n");
	}
	else {
		process *tmp = head;
		while (tmp != NULL){
			char cmdprint[128];
			int icommand=0;
			int iprint=0;
			while ((tmp->command)[icommand] != NULL) {
				for (int i=0; i < strlen((tmp->command)[icommand]); i++){
					cmdprint[iprint] = (tmp->command)[icommand][i];
					iprint++;
				}
				cmdprint[iprint] = ' ';
				iprint++;
				icommand++;
			}
			cmdprint[iprint]='\0';
			if (tmp->state==PAUSED){
				printf("Process ID: %d, Command: %s, State: paused\n",tmp->pid,cmdprint);
			}
			else {
				printf("Process ID: %d, Command: %s, State: running\n",tmp->pid,cmdprint);
			}
			tmp = tmp->next;
		}
	}
}

int command_check(char **dir_list,char ***one_command){
	int i = 0;
	int rv;
	while (dir_list[i] != NULL){
		int length = strlen(dir_list[i]) + strlen((*one_command)[0]) + 2;
		char* new_command = malloc(sizeof(char) * length);
		int j = 0;
		while (j < strlen(dir_list[i])){
			new_command[j] = dir_list[i][j];
			j++;
		}
		new_command[j] = '/';
		j++;
		int k = 0;
		while (j < length - 1){
			new_command[j] = (*one_command)[0][k];
			j++;
			k++;
		}
		new_command[j] = '\0';
		struct stat statresult;
		rv = stat(new_command,&statresult);
		//if stat found something, replace original with full path
		if (rv >= 0){
			char *tmp = (*one_command)[0];
			(*one_command)[0] = new_command;
			free(tmp);
			break;
		}
		else {
			free(new_command);
		}
		i++;
	}
	return rv;
}

void sequential (int * new_mode, int *ex, char **commands, char ** dir_list){
	char **one_command;
	int i = 0;
	int special = 0;
	while (commands[i] != NULL){
		one_command = tokenify(commands[i]," \n\t");
		if (one_command[0] == NULL) {
			special = 1;
		}
		if (strcmp(one_command[0],"exit") == 0) {
			*ex=1;
		}
		else if ((strcmp(one_command[0], "mode")) == 0) {
			if (one_command[1] != NULL) {
				if ((strcmp(one_command[1], "sequential")) == 0 ||
							(strcmp(one_command[1], "s")) == 0) {
					*new_mode = SEQUENTIAL;
				}
				else if ((strcmp(one_command[1], "parallel")) == 0 ||
							(strcmp(one_command[1], "p")) == 0) {
					*new_mode = PARALLEL;
				}
				else {
					printf("Invalid mode, please try again.\n");
				}
			}
			else {
					printf("Current mode: sequential\n");
			}
			special = 1
		}
		else if (strcmp(one_command[0],"jobs") == 0){
			printf("No parallel jobs running in sequential mode.\n");
			special = 1;
		}
		else if (strcmp(one_command[0],"pause")==0){
			printf("Pause is not a valid command in sequential mode\n");
			special = 1;
		}
		else if (strcmp(one_command[0],"resume")==0){
			printf("Resume is not a valid command in sequential mode\n");
			special = 1;
		}
		if (special) {
			special = 0;
			free_array(one_command);
			i++;
			continue;
		}
		struct stat statresult;
		int rv = stat(one_command[0], &statresult);
		if (rv < 0){
			//stat failed, no such file
			//find another one and change it
			rv = command_check(dir_list, &one_command);
			if (rv < 0){
				printf("Invalid command: %s\n", one_command[0]);
				i++;
				continue;
			}
		}
		int child_pid = fork();
		int child_status;
		if (child_pid < 0){
			fprintf(stderr,"fork failed, could not create child process\n");
		}
		else if (child_pid == 0) {
			if (execv(one_command[0],one_command)<0){
				fprintf(stderr,"Execution failed: %s\n",strerror(errno));
			}
		}
		else {
			waitpid(child_pid, &child_status, 0);
			free_array(one_command);
		}
	i++;
	}
	free_array(commands);
}

void start_prompt(char ** dir_list) {
	char buffer[1024];
	int ex = 0;
	int mode = SEQUENTIAL;
	int new_mode = mode;
	process *head = NULL;
	char **one_command;

	while (fgets(buffer, 1024, stdin) != NULL) {
		buffer[strlen(buffer)-1] = '\0';
		char *comment = NULL;
		if ((comment = strchr(buffer, '#')) != NULL) {
			*comment = '\0';
		}
		char **commands = tokenify(buffer, ";");

		if (mode == SEQUENTIAL) {
			sequential(&new_mode, &ex, commands, dir_list);
		}
		// else: PARALLEL MODE STARTS
		else {
			int cont = 1;
			int i = 0;
			int special = 0;
			while (cont) {
				while(commands[i] != NULL) {
					one_command = tokenify(commands[i], " \n\t");
					if (one_command[0] == NULL) {
						special = 1;
					}
					if ((strcmp(one_command[0], "exit")) == 0) {
						ex = 1;
						special = 1;
					}
					if ((strcmp(one_command[0], "jobs")) == 0) {
						print_process(head);
						special = 1;
					}
				
					if (((strcmp(one_command[0], "pause")) == 0) ||
						((strcmp(one_command[0], "resume")) == 0)) {
						if (one_command[1] != NULL) {
							pid_t pid = atoi(one_command[1]);
							process *process_found = find_pid(head, pid);
							if (process_found != NULL) {
								if (strcmp(one_command[0], "pause") == 0) {
									kill(process_found->pid, SIGSTOP);
									process_found -> state = PAUSED;
								}
								else {
									kill(process_found->pid, SIGCONT);
									process_found -> state = RUNNING;
								}
							}
							else {
								printf("Cannot find process %d\n", pid);
							}
						}
						else {
							printf("Please call resume or pause with a PID.\n");
						}
						special = 1;
					}
				
					if ((strcmp(one_command[0], "mode")) == 0) {
						if (one_command[1] != NULL) {
							if ((strcmp(one_command[1], "sequential")) == 0 ||
										(strcmp(one_command[1], "s")) == 0) {
								new_mode = SEQUENTIAL;
								cont = 0;
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
							printf("Current mode: parallel\n");
						}
						special = 1;
					}
					
					if (special) {
						special = 0;
						free_array(one_command);
						i++;
						continue;
					}
				
					struct stat statresult;
					int rv = stat(one_command[0], &statresult);
					if (rv < 0){
						//stat failed, no such file
						//find another one and change it
						rv=command_check(dir_list, &one_command);
						if (rv < 0){
							printf("Invalid command: %s\n", one_command[0]);
							i++;
							continue;
						}
					}
					
					int child_pid = fork();
					
					if (child_pid < 0) {
						fprintf(stderr, "fork failed, could not create child process\n");
					} 
					else if (child_pid == 0) {
					// child: execv this command 
						if (execv(one_command[0],one_command) < 0) {
							fprintf(stderr, "execv failed: %s\n", strerror(errno));
						}
					}
					else if (child_pid > 0) {
					// parent
						head = add_process(head, child_pid, one_command, RUNNING);
					}
					i++;
				}
				
				// after first round of forking, parent poll for new commands
				// declare an array of a single struct pollfd
				free_array(commands);
				if (cont == 1) {
					pid_t result;
					int status;
				    struct pollfd pfd[1];
				    pfd[0].fd = 0; // stdin is file descriptor 0
				    pfd[0].events = POLLIN;
				    pfd[0].revents = 0;
					// wait for input on stdin, up to 500 milliseconds
				    // the return value tells us whether there was a 
				    // timeout (0), something happened (>0) or an
				    // error (<0).
				    	
					while (1) {
						result = waitpid(-1, &status, WNOHANG);
						if (result > 0) {
							process *temp = find_pid(head, result);
							printf("Process %d (%s) completed.\n", temp->pid, *(temp->command));
							head = delete_process(head, temp->pid);
						}
						if (head == NULL && ex == 1) {
							free_array(dir_list);
							exit(0);
						}
						int return_val = poll(&pfd[0], 1, 500);
						if (return_val == -1) {
							perror("poll"); // error occurred in poll()
						} 
						if (return_val > 0) {
							char temp[1024];
							read(pfd[0].fd, temp, sizeof(buffer));
							char *null = NULL;
							if ((null = strchr(temp, '\n')) != NULL) {
								*null = '\0';
							}
							strncpy(buffer,temp,strlen(temp)+1);
							char *comment = NULL;
							if ((comment = strchr(buffer, '#')) != NULL) {
								*comment = '\0';
							}
						    commands = tokenify(buffer, ";");
						    i = 0;
							break;
						}
					}
				}
			}
		}
		while (wait(NULL) > 0); // parallel stage 1 || when cont = false
		// exit after finishing all commands
		if (ex == 1) {
			free_array(dir_list);
			exit(0);
		}
		if (new_mode != mode) {
			mode = new_mode;
		}
		printf("%s", PROMPT);
		fflush(stdout);
	}
	printf("\n"); // EOF reached, just print new line before exiting
}

int main(int argc, char **argv) {
	char **dir_list = load_directories("./shell-config");
	printf("%s", PROMPT);
	fflush(stdout);
	start_prompt(dir_list);
	free_array(dir_list);
    return 0;
}
