#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
// Constants Decloartions
#define MAX_HOST_NAME 256
#define MAX_PATH_LENGTH 256
#define MAX_LOGIN_NAME 256
#define MAX_ARG_COUNT 128
#define MAX_PROMPT_LENGTH 4096
// Struct declorations
typedef struct bg_process {
    pid_t pid;                
    char *command;       
    struct bg_process *next;
} bg_process;
// Function Prototypes
char *create_prompt();
char *read_imput(const char *prompt);
char **tokenize_args(char* reply, char *deliminator);
void change_directory(char *desination);
void foreground_execution(const char *program, char *args[]);
void background_execution(const char *program, char *args[], bg_process **bg_list);
void add_bgprocess_tolist(bg_process **bg_list, pid_t pid,char *args[]);
void display_bg_list(bg_process *bg_list);
void dont_exit_on_CTRL(int sig);
// Creates the prompt from the system (login@machine: path >)
char* create_prompt(){
	char *login_name;
    char hostname[MAX_HOST_NAME];
    char cwd[MAX_PATH_LENGTH];
    char *prompt = NULL;
    size_t prompt_length = 0;

    login_name = getlogin();
    if (login_name == NULL) {perror("getlogin() error");exit(1);}

    if (gethostname(hostname, MAX_HOST_NAME) != 0) {perror("gethostname() error");exit(1);}

    if (getcwd(cwd, MAX_PATH_LENGTH) == NULL) { perror("getcwd() error");exit(1);}

    prompt_length = strlen(login_name) + strlen(hostname) + strlen(cwd) + 5;

    prompt = (char*)malloc(prompt_length);
    if (prompt == NULL) {printf("Memory allocation failed");exit(1);}

    strcpy(prompt, login_name);   
    strcat(prompt, "@");          
    strcat(prompt, hostname);     
    strcat(prompt, ": ");        
    strcat(prompt, cwd);         
    strcat(prompt, " >");         

    return prompt;
}
// Reads the user imput 
char *read_imput(const char *prompt){
	 char* reply = readline(prompt);

	 if (reply==NULL) {return NULL;}

	 return reply;
}
// Tokenizes any string passed based on the deliminator
char **tokenize_args(char *reply, char *deliminator){
	char **args = malloc(MAX_ARG_COUNT * sizeof(char*));
	char *token;

	if (args == NULL){ perror("memory allocation fail");}
	token = strtok(reply,deliminator);

	int i = 0;
	while(token != NULL){ 
		args[i] = token;
		token = strtok(NULL,deliminator );
		i++;
	} 
	return args;
}
// Function to change directoy
void change_directory(char *destination){
	char cwd[MAX_PATH_LENGTH];
	char new_directory[MAX_PATH_LENGTH];
	if (getcwd(cwd, MAX_PATH_LENGTH) == NULL) { perror("getcwd() error");exit(1);}

	// relative directory change
	 if (destination == NULL || strcmp(destination, "") == 0) {
		char *home_dir = getenv("HOME");
			if(home_dir == NULL){
				perror("bash: cd:");
				exit(1);
			}
		strcpy(new_directory,home_dir);
	}
	else if (destination[0] != '/'){

		if(destination[0] == '~'){
			char *home_dir = getenv("HOME");
				if(home_dir == NULL){
					perror("bash: cd:");
					exit(1);
				}
				strcpy(new_directory,home_dir);
				strcat(new_directory,destination + 1);
		}else{
			strcat(new_directory,cwd);
			strcat(new_directory,"/");
			strcat(new_directory,destination);
		}
	} 
	else{
		strcpy(new_directory, destination); 
	}
  	if(chdir(new_directory) != 0){
		perror("bash: cd:");
	}
}
// function to execute programs in the foreground
void foreground_execution(const char *program, char *args[]){

	if(program == NULL || args == NULL || args[0] == NULL){
		return;
	}
	pid_t pid = fork();

	if(pid == 0){
		if (execvp(program, args)==-1){
    	fprintf(stderr, "%s: %s\n", args[0], strerror(errno));
			_exit(1);}
	}
	else{ // wait for child to finish executing
		int status;
		waitpid(pid, &status, 0);
	}
}
// function to exxecute programs in the back ground
void background_execution(const char *program, char *args[], bg_process **bg_list){

	if(program == NULL || args == NULL || args[0] == NULL){
		return;
	}

	pid_t pid = fork();

	if (pid < 0){
		perror("Fork failed:");
		return;
	}
	else if (pid == 0) {
        // saw this from a comment from a TA on teams, not exactly sure if we are suppose to display output of BG task to terminal
        // so I am redirecting it to /dev/null as to not print it
        int dev_null_fd = open("/dev/null", O_WRONLY);
		dup2(dev_null_fd, STDOUT_FILENO);  // Redirect stdout
        dup2(dev_null_fd, STDERR_FILENO);  // Redirect stderr
        close(dev_null_fd);

        if (execvp(program, args) == -1) {
            perror("Failed to execute background command");
            _exit(-1);  
        }

	}
	else{
		add_bgprocess_tolist(bg_list, pid, args);
	}


}
// adds a background processes information to a node in a linked list
void add_bgprocess_tolist(bg_process **bg_list, pid_t pid,char *args[]){
	// initalizations and assingmnets
	bg_process *newprocess = (bg_process*)malloc(sizeof(bg_process));
	if (newprocess == NULL){
		exit(1);
	}
	int args_length = 0;
	newprocess->pid = pid;
	newprocess->next = NULL;
	// memory allocation and string copying
	for (int i = 0; args[i] != NULL; i++) {
      args_length += strlen(args[i]) + 1;
    }
	newprocess->command = (char*)calloc((args_length+1),sizeof(char)); //leads to undefined behaviors when memory is allocated using malloc
	if (newprocess->command == NULL){
        perror("Memory allocation failed");
		exit(1);
	}
    newprocess->command[0] = '\0'; // ensure start of string is empty for strcat to work properly
	for (int i = 0; args[i] != NULL; i++) {
        strcat(newprocess->command, args[i]);
        if (args[i + 1] != NULL) {
            strcat(newprocess->command, " ");  
        }
    }
	// linked list insertion logic
    if (*bg_list == NULL) {
        *bg_list = newprocess;  
    } else {
        bg_process *temp = *bg_list;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = newprocess;  
    }

}
// prints all of the currently active bg processes
void display_bg_list(bg_process *bg_list) {
    bg_process *temp = bg_list;
    int count = 0;
    while (temp != NULL) {
        printf("%d: %s\n", temp->pid, temp->command);
        count++;
        temp = temp->next;
    }
    printf("Total Background jobs: %d\n", count);
}
// check to see if a packground process has terminated
void check_bg_processes(bg_process **bg_list) {
    bg_process *temp = *bg_list;
    bg_process *prev = NULL;
    int status;
    pid_t result;

    while (temp != NULL) {
        result = waitpid(temp->pid, &status, WNOHANG);

        if (result == 0) {
            prev = temp;
            temp = temp->next;
        } else if (result == temp->pid) {
            printf("%d: %s has terminated.\n", temp->pid, temp->command);

            if (prev == NULL) {
                *bg_list = temp->next; 
            } else {
                prev->next = temp->next;
            }
            if (temp->command != NULL) {
                free(temp->command); 
            }
            bg_process *free_node = temp; 
            temp = temp->next;
            free(free_node); 
        } else {
            perror("error");
            exit(1);
        }
    }
}
void dont_exit_on_CTRL(int sig){
    //do nothing
}


int main()
{
	int bailout = 0;
	bg_process *bg_list = NULL;
    signal(SIGINT,dont_exit_on_CTRL);


	while (!bailout)
	{
		char *prompt = create_prompt();
		char* reply = read_imput(prompt);
		char **args = tokenize_args(reply," ");

		if (reply == NULL || strlen(reply) == 0) {
		free(reply);
		free (args);
		free(prompt);
		continue;
		}
		// user command case logic
		if (!strcmp(args[0], "exit"))
		{
			bailout = 1;
		}
		else
		{
			if (!strcmp(args[0], "cd")){
				change_directory(args[1]);
			}
			else if (!strcmp(args[0], "bg")){
				background_execution(args[1], args+1, &bg_list);
			}
			else if (!strcmp(args[0], "bglist")){
				display_bg_list(bg_list);
			}
			else{
				foreground_execution(args[0], args);
			}
			check_bg_processes(&bg_list);
		}
		// making sure all the memroy is freed
		free(args);
		free(prompt);
		free(reply);
	}
	//make sure list is freed
	bg_process *temp;
	while (bg_list != NULL) {
        temp = bg_list;           
        bg_list = bg_list->next;   
        if (temp->command != NULL) {
            free(temp->command);
        }
        free(temp);
    }
	return 0;
}
