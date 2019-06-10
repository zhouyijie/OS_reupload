#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

int execvp(const char *file, char *const argv[]);
void initialize(char *args[]);

pid_t wpid = -1 ; //global
pid_t pid = -1 ;
int status = 0;
int dup_fd;

//struct
int commandNumber = 0;
struct node *head_job = NULL;
struct node *current_job = NULL;

struct node {
	int number; // the job number
	int pid; // the process id of the a specific process
	struct node *next; // when another process is called you add to the end of the linked list
};

//initialize args array
void initialize(char *args[]) {
	for (int i = 0; i < 20; i++) {
		args[i] = NULL;
	}
	return;
}

//signal handler
void sigHandler(int sig){
	wpid = waitpid(pid, &status,0);//terminate child process
	//kill(pid,SIGKILL);
	printf("\nkilled the program\nUsing signal Handler interrupt\n");
	exit(1);
	return;
}

//jobs implementation
//display the list
void printList() {
	struct node *list = head_job;
	printf("\ne.g. [(number,process id)]\nusing number in fg commands for specific pid");

	printf("\n[ ");
	 
	//start from the beginning
	while(list != NULL) {
	   printf("(%d,%d) ",list->number,list->pid);
	   list = list->next;
	}
	 
	printf(" ]");
 }

 //find a link with given key
struct node* find(int key) {
	
	//start from the first link
	struct node* current = head_job;
	
	//if list is empty
	if(head_job == NULL) {
		return NULL;
	}
	
	//navigate through list
	while(current->number != key) {
		
		//if it is last node
		if(current->next == NULL) {
			return NULL;
		} else {
			//go to next link
			 current = current->next;
		}
	}      
		
	//if data found, return the current Link
	return current;
}



//build in
//function declarations for shell build in commands

int buildin_cd(char **args);
int buildin_pwd(char **args);
int buildin_exit(char **args);
int buildin_fg(char **args);
int buildin_ls(char **args);
int buildin_cp(char **args);
int buildin_cat(char **args);
int buildin_jobs(char **args);


//list of built in commands
char *buildin_str[] = { "cd", "pwd", "exit", "fg", "ls","jobs","cat","cp" };

int(*buildin_funct[]) (char **) = { &buildin_cd, &buildin_pwd, &buildin_exit, &buildin_fg , &buildin_ls, &buildin_jobs, &buildin_cat, &buildin_cp};

int num_buildins() {
	return sizeof(buildin_str) / sizeof(char *);
}

/*
	Buildin function implementation
*/

int buildin_cat(char **args){
	
	
	if(args[2]){
		printf("initiate redirect\n");
		
		//output redir
		int fd_1 = open(args[3],  O_WRONLY | O_CREAT, 0666);

		
		//int stdout_copy = dup(STDOUT_FILENO);
					
		close(1);
		dup_fd = dup(fd_1);
		
		
		
		//nullify arguments
		args[2] = NULL;
		args[3] = NULL;
		
		
		
	}
	
	
	printf("cat commands executed\n");
	execvp(args[0],args);
	return 1;

}
int buildin_cp(char **args){
	printf("cp commands executed\n");
	execvp(args[0],args);
	return 1;

}

int buildin_jobs(char **args){
	printf("jobs commands executed\n");
	
	printf("now pid is %d\nif pid is zero, parent process is waiting for this child process\n",pid);
	printList();
	return 1;
}

int buildin_ls(char **args){
	
	//printf("call ls \n");
	
	execvp(args[0], args);
	
	return 1;
}

int buildin_cd(char **args){
	int result = 0;
	if(args[1] == NULL) {
		fprintf(stderr, "argument expected to cd commands\n return $Home variable declared in the environment\n");
		char *home = getenv("HOME");
		if (home != NULL) {
			result = chdir(home);
		}
		else {
			printf("cd: No $HOME variable declared in the environment");
		}
	}else {
		result = chdir(args[1]);
	}
	if (result == -1) fprintf(stderr, "cd: %s: No such file or directory", args[1]);
	return 1;
}

int buildin_pwd(char **args){
	if(0) {
		fprintf(stderr, "argument expected to pwd commands\n");
	}else {
		char pwd[256];
		printf("%s\n", getcwd(pwd, sizeof(pwd)));
	}
	return 1;
}

int buildin_exit(char **args){
	printf("exit now\n");
	exit(1);
	
	return 1;
}

int buildin_fg(char **args){
	
	if (args[1]) {
		// Switch to a job
		int job_num = atoi(args[1]);
		printf("Switching to process %d.\n", job_num);
		struct node* fg = find(job_num);
		int pid_seek = fg->pid;
		if ((waitpid(pid_seek, NULL, 0))==-1) {
			printf("switched to process id: %d.\n", pid_seek);
			//*errorStatus = 1;
		}else{
			printf("invalid number\n");
		}
	} 
	else {
			printf("Please provide a process id argument for \"fg\"\n");
	}

	return 1;
}

int getcmd(char *prompt, char *args[], int *background)
{
	pid_t pid;
	pid_t wpid;
	int status;


	int length, i = 0;
	char *token, *loc;
	char *line = NULL;
	size_t linecap = 0;

	printf("%s", prompt);
	length = getline(&line, &linecap, stdin);

	if (length <= 0) {
		exit(-1);
	}
	// Check if background is specified..
	if ((loc = index(line, '&')) != NULL) {
		*background = 1;
		*loc = ' ';
	} else
		*background = 0;
	while ((token = strsep(&line, " \t\n")) != NULL) {
	for (int j = 0; j < strlen(token); j++)
		if (token[j] <= 32)
			token[j] = '\0';
	if (strlen(token) > 0)
		args[i++] = token;
	}
	return i;
}

int main(void)
{
	char *args[20];
	int bg;
	int i;

	char *user = getenv("USER");
	if (user == NULL) user = "User";

	char str[sizeof(char)*strlen(user) + 4];
	sprintf(str, "\n%s>> ",user);

	printf("buildin commands: ");
	for (i = 0;i < num_buildins();i++){
		printf("%s, ",buildin_str[i]);
	}
	printf("\n");

	//binding signal handler

	if ( signal(SIGINT,(void (*)(int))sigHandler) == SIG_ERR){
		printf("Error, could not bind the signal handler\n");
		exit(1);

	}
	if (signal(SIGTSTP, SIG_IGN) == SIG_ERR){
		printf("Error, could not ignore ctr+z\n");
	} //IGNORE ctr+Z

	while(1) {
		initialize(args);
		bg = 0;
		int cnt = getcmd(str, args, &bg);
		if(strcmp(args[0],"exit")==0){
			printf("exit this process\nif all processes exited, program terminated\nctr+Z disabled\npress ctr+C to use interrupt signal to kill the program\n");
			exit(-1);
		}
		
		

		pid = fork();
		if (pid == 0){
			//Child process

			for(i = 0; i < num_buildins(); i++) {
				if (strcmp(args[0],buildin_str[i]) == 0) {
					buildin_funct[i](args);
				}
			}
			/*
			//output redir
			int fd_1 = open("out.txt",  O_RDONLY, 0);
			
			int stdout_copy = dup(STDOUT_FILENO);
						
			close(STDOUT_FILENO);
			int dup_fd = dup(fd_1);
			close(dup_fd);
			*/
		}else{
			
			
			wpid = waitpid(pid, &status, WUNTRACED);
			printf("now pid is %d\n",pid);
			
			
			
			struct node *job = malloc(sizeof(struct node));
			
			//If the job list is empty, create a new head
			if (head_job == NULL) {
				job->number = 1;
				job->pid = pid;
				//the new head is also the current node
				job->next = NULL;
				head_job = job;
				current_job = head_job;
			}
			
			//Otherwise create a new job node and link the current node to it
			else {
			
				job->number = current_job->number + 1;
				job->pid = pid;
				current_job->next = job;
				current_job = job;
				job->next = NULL;
			}

		}
		//build in functions
		

		

		/*

		pid = fork();		
		if (pid == 0){
			//child process

			
			//printf("child process\n");
			
			//output redir
			int fd_1 = open("out.txt",  O_RDONLY, 0);

    		int stdout_copy = dup(STDOUT_FILENO);
    		
    		close(STDOUT_FILENO);
    		int dup_fd = dup(fd_1);
    		close(dup_fd);

			//execvp system call
			
			execvp(args[0],args);
			
			exit(EXIT_FAILURE);
			
			
		}else if (pid < 0){
			//error forking
			printf("error ecountered during fork\n");
			exit(EXIT_FAILURE);
		}else {
			//printf("parent process, child process ID = %zu\n", (size_t) pid);
			//parent process
			//waitpid()
			//int status;
			wpid = waitpid(pid, &status, WUNTRACED);
			if (wpid == (pid_t) -1){
				printf("Error has occurred\n");
				exit(EXIT_FAILURE);
			}
			if (WIFSTOPPED(status)){
				printf("child process have stopped\n");
			} 
			//child process terminated
			//wpid = waitpid(pid, &status,0);
			//child exit with code:
			//WEXITSTATUS(status);
		}
		*/
	
	}
}





