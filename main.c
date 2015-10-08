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
struct stat;
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
int command_check(char **dir_list,char *** one_command){
	int i=0;
	int rv;
	while (dir_list[i]!=NULL){
		int length=strlen(dir_list[i])+strlen((*one_command)[0])+2;
		char* new_command=(char*) malloc(sizeof(char)*length);
		int j=0;
		while (j<strlen(dir_list[i])){
			new_command[j]=dir_list[i][j];
			j++;
		}
		new_command[j]='/';
		j++;
		int k=0;
		while (j<length-1){
			new_command[j]=(*one_command)[0][k];//*(*command + k);
			j++;
			k++;
		}
		new_command[j]='\0';
		struct stat statresult;
		rv=stat(new_command,&statresult);
		if (rv>=0){
			char * tmp = (*one_command)[0];
			(*one_command)[0]=new_command;
			free(tmp);
			break;
		}else{
			free(new_command);
		}
		i++;
	}
	return rv;
}
void start_prompt(char ** dir_list) {
	char buffer[1024];
	int ex = 0;
	int mode = SEQUENTIAL;
	int new_mode = mode;
	while (fgets(buffer, 1024, stdin) != NULL) {
		buffer[strlen(buffer)-1] = '\0';
		char *comment;
		if ((comment = strchr(buffer, '#')) != NULL) {
			*comment = '\0';
		}		
		
		int i = 0;
		char **commands = tokenify(buffer, ";");
		char **one_command;
		while (commands[i] != NULL) {
			one_command = tokenify(commands[i], " \n\t");
			
			if (one_command[0] == NULL) {
				free_tokens(one_command);
				i++;
				continue;
			}
			
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
					if (mode == SEQUENTIAL) { 
						printf("Current mode: sequential\n");
					}
					else {
						printf("Current mode: parallel\n");
					}
				}
				i++;
				continue;
			}
			
			struct stat statresult;
			int rv=stat(one_command[0],&statresult);
			if (rv<0){
				//stat failed, no such file
				//find another one and change it
				rv=command_check(dir_list,&one_command);
				if (rv<0){
					printf("Invalid command: one_command[0]\n");
					continue;
				}
			}
			
			pid_t tpid;
			int child_pid = fork();
			//if parallel: add into struct process into linked list(insert, delete)
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
				//printf("not in here");
				do {
					tpid = wait(&child_status);
					if (tpid == child_pid) { // if child terminated
						free_tokens(one_command);
						break;
					}
				} while(tpid != child_pid);
			}
			if (mode == PARALLEL) {
				free_tokens(one_command);
			}
			i++;
		}
		while (wait(NULL) > 0); // parallel
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

char ** load_directories(){
	FILE * dir_file=fopen("./shell-config","r");
	char** result=(char**)malloc(sizeof(char*)*8);
	result[7]=NULL;
	char next[32];
	int i=0;
	while (fgets(next,32,dir_file)!=NULL){
		printf("%s\n",next);
		int k=strlen(next)-1;
		next[k]='\0';
		result[i]=strdup(next);
		i++;
	}
	fclose(dir_file);
	return result;
}
void free_directories(char** dir_list){
	int i=0;
	while(dir_list[i]!=NULL){
		free(dir_list[i]);
		i++;
	}
	free(dir_list);
}

int main(int argc, char **argv) {
	//char **dir_list=load_directories();
	char ** dir_list=(char **)malloc(sizeof(char*)*8);
	dir_list[0]=strdup("/bin");
	dir_list[1]=strdup("/usr/bin");
	dir_list[2]=NULL;
	
	
	//int i=0;
	//while (dir_list[i]!=NULL){
		//printf("%s\n",dir_list[i]);
		//i++;
	//}
	printf("%s", PROMPT);
	fflush(stdout);
	start_prompt(dir_list);
	//free_directories(dir_list);
    return 0;
}
