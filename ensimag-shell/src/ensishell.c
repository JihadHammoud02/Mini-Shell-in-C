/*****************************************************
 * Copyright Grégory Mounié 2008-2015                *
 *           Simon Nieuviarts 2002-2009              *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "variante.h"
#include <sys/wait.h>
#include "readcmd.h"
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>
#ifndef VARIANTE
#error "Variante non défini !!"
#endif

/* Guile (1.8 and 2.0) is auto-detected by cmake */
/* To disable Scheme interpreter (Guile support), comment the
 * following lines.  You may also have to comment related pkg-config
 * lines in CMakeLists.txt.
 */
struct background_processes *head = NULL;
#if USE_GUILE == 1
#include <libguile.h>

// static int received_SIGCHILD = 0;

struct background_processes{
	int pid;
	char* cmd_name;
	struct background_processes * next;
};

// Add a background process to the Background processes LinkedList
void add_background_process(int pid, const char *name) {
    // Allocate and initialize a new node
    struct background_processes *node = malloc(sizeof(*node));
    if (node == NULL) {
        exit(EXIT_FAILURE);
    }
    node->pid = pid;
    node->cmd_name = strdup(name);
    node->next = NULL;

    // Append the new node to the list
    if (head == NULL) {
        head = node;
    } else {
        struct background_processes *current = head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = node;
    }
}

int is_background_process( int pid){
	struct background_processes *current = head;
	int verification=0;
	while(current != NULL && verification == 0){
		if( current->pid == pid){
			verification = 1;
		}
		current=current->next;
	}
	return verification;
}

// Remove a background process from the Background processes LinkedList
void remove_background_process(int pid) {
    struct background_processes *current = head;
    struct background_processes *prev = NULL;
    while (current != NULL) {
        struct background_processes *next = current->next;
        // If the process is running
        if (pid == current->pid) {
            // detach the node
            if (current == head) {
                head = next;
            } else {
                prev->next = next;
            }
            // deallocate the node
            free(current->cmd_name);
            free(current);
            current = next;
        } else {
            prev = current;
            current = next;
        }
    }
}


void display_all_background_processes() {
    struct background_processes *current = head;
    printf("PID\t CMD\t \n");
    while (current != NULL) {
		printf("%d\t %s\t \n",current->pid , current->cmd_name);
		current=current->next;
        }

}


void handle_input_redirection(char *input_file){
	// Redirect the content of the file as input to the command cat
	int read_this_fd=open(input_file, O_RDONLY);
	dup2(read_this_fd , 0);
	close(read_this_fd);
}

void handle_output_redirection(char* output_file){
	// Redirect the stdout to the file ,set the permissions , create the file if not available: O_CREAT , delete the content of the file if available: O_TRUNC
	int write_this_fd = open(output_file,O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	dup2(write_this_fd,1);
	close(write_this_fd);
}

void execute_single_command(char **cmd){
	execvp(cmd[0],cmd);
}


void jobs_command(){
	display_all_background_processes();
}

void free_linkedlist(void) {
    struct background_processes *current = head;
    while (current != NULL) {
        struct background_processes *next = current->next;
        free(current->cmd_name);
        free(current);
        current = next;
    }
}

void SIGCHILD_handler() {
    int status;
    pid_t child_pid = waitpid(-1, &status, WNOHANG);
    if (child_pid > 0 && is_background_process(child_pid ) == 1) {
		printf("Process %d terminated \n",child_pid);
        remove_background_process(child_pid);
		exit(0);
	}
}


int Number_of_child_processes(struct cmdline *l){
	int number_of_child_processes = 0;
	int i=0;
	while(l->seq[i] != 0){
		number_of_child_processes++;
		i++;
	}
	return number_of_child_processes;
}

void execute_pipeline(struct cmdline *l){     // *head_over=head // *head - > node 1
	int previous_pipe_read = -1;
	int file_descriptor[2];
	int number_of_childs=Number_of_child_processes(l);
	int counter=0;
	for(size_t i=0 ; l->seq[i] != 0; i++){
		pipe(file_descriptor);

		pid_t child_pid = fork();
		char**cmd = l->seq[i];
		

		if (child_pid == -1) {
            perror("fork error");
            exit(EXIT_FAILURE);
        }
		

		if(child_pid == 0){
			// Child process
			// If background process add it to the linked list of the background processes

			if (l->in != NULL) {
                handle_input_redirection(l->in);
            }

            if (l->out != NULL) {
                handle_output_redirection(l->out);
            }
			
			if(strcmp(cmd[0], "jobs") == 0){
			jobs_command();
			exit(0);
			}

			if (previous_pipe_read != -1)
			{
				// Redirect  to the reading side of the pipe ( file_descriptor[0] ) and use it as stdint
				dup2(previous_pipe_read, 0);
				close(previous_pipe_read);
			}

			if (l->seq[i + 1] != 0)
			{
				// Redirect stdout to the writing side of the pipe ( file_descriptor[1] )
				dup2(file_descriptor[1], 1);
			}
			// Close unecessary file descriptor
			close(file_descriptor[0]);
			// Execute command
			execute_single_command(cmd);
		}
		else{

			// Parent process
			// Handle the signal received by a terminated child process
			close(file_descriptor[1]); // Close unecessary file descriptor
			
			if (previous_pipe_read != -1)
			{
				close(previous_pipe_read);
			}


			if (!l->bg)
			{
				int status;
				waitpid(child_pid, &status, 0);
			}
			else{
				add_background_process(child_pid,cmd[0]);
			}

			// save the reading side of the pipe (aka the ouput of the previous command) into the variable previous_pipe_read
			previous_pipe_read = file_descriptor[0];
			
		}

		counter++;



	}


}

int question6_executer(char *line)
{
	/* Question 6: Insert your code to execute the command line
	 * identically to the standard execution scheme:
	 * parsecmd, then fork+execvp, for a single command.
	 * pipe and i/o redirection are not required.
	 */
	struct cmdline *l;
	l = parsecmd(&line);
	execute_pipeline(l);

	/* Remove this line when using parsecmd as it will free it */
	free(line);

	return 0;
}

SCM executer_wrapper(SCM x)
{
	return scm_from_int(question6_executer(scm_to_locale_stringn(x, 0)));
}
#endif

void terminate(char *line)
{
#if USE_GNU_READLINE == 1
	/* rl_clear_history() does not exist yet in centOS 6 */
	clear_history();
#endif
	if (line)
		free(line);
	printf("exit\n");
	exit(0);
}



int main()
{
	printf("Variante %d: %s\n", VARIANTE, VARIANTE_STRING);

#if USE_GUILE == 1
	scm_init_guile();
	/* register "executer" function in scheme */
	scm_c_define_gsubr("executer", 1, 0, 0, executer_wrapper);
#endif

	while (1)
	{
		struct cmdline *l;
		char *line = 0;
		char *prompt = "ensishell>";
		signal(SIGCHLD, SIGCHILD_handler);

		/* Readline use some internal memory structure that
		   can not be cleaned at the end of the program. Thus
		   one memory leak per command seems unavoidable yet */
		line = readline(prompt);
		if (line == 0 || !strncmp(line, "exit", 4))
		{
			terminate(line);
		}

#if USE_GNU_READLINE == 1
		add_history(line);
#endif

#if USE_GUILE == 1
		/* The line is a scheme command */
		if (line[0] == '(')
		{
			char catchligne[strlen(line) + 256];
			sprintf(catchligne, "(catch #t (lambda () %s) (lambda (key . parameters) (display \"mauvaise expression/bug en scheme\n\")))", line);
			scm_eval_string(scm_from_locale_string(catchligne));
			free(line);
			continue;
		}
#endif

		/* parsecmd free line and set it up to 0 */
		l = parsecmd(&line);

		/* If input stream closed, normal termination */
		if (!l)
		{

			terminate(0);
		}

		if (l->err)
		{
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}

		if (l->in)
			printf("in: %s\n", l->in);
		if (l->out)
			printf("out: %s\n", l->out);
		if (l->bg)
			printf("background (&)\n");

		/* Display each command of the pipe */
		execute_pipeline(l);
	}
	free_linkedlist();
}
