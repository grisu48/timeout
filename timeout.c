
/* =============================================================================
 * 
 * Title:       timeout - Execute program with timeout
 * Author:      Felix Niederwanger
 * License:     MIT license
 * Description: timeout executes a given command with a given timeout.
 *              When the timeout exceedes, the program terminates the executed
 *              program
 * =============================================================================
 */

#define VERSION "1.1"


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <ctype.h>




static unsigned int timeout = 0L;		// Timeout for the comamnd in seconds
static char* command = NULL;			// Command
static bool kill9 = false;				// Use SIGKILL instead of SIGTERM
static pid_t proc_pid = 0;				// Child process id
static long runtime = 0L;				// Current runtime
static bool verbose = false;			// Verbosity flag
static bool daemonize = false;			// Daemonize program

/* ==== Prototypes ========================================================== */

/** Fork program to run as daemon*/
static void fork_daemon(void);
/** Print help message */
static void printHelp(const char* progname);
/** Terminate child progress*/
static void terminate_process(void);
/* signal handler */
static void sig_handler(int sig_no);


/* ==== Usefull functions =================================================== */

/** Checks if the given string is a number or not */
static bool is_numeric(const char *s) {
	if (s == NULL || *s == '\0' || isspace(*s)) return 0;
	char * p;
	strtod (s, &p);
	return *p == '\0';
}

/** Milliseconds since epoc */
static long millis(void) {
	struct timeval  tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec) * 1000L + (tv.tv_usec) / 1000L;
}

/** Append c2 to c1 with a space and frees c1 */
static char* strappend(char* c1, char* c2) {
	char* result;
	if(c1 == NULL) {
		const size_t len = strlen(c2);
		result = (char*)malloc(sizeof(char)*(len+1));
		strncpy(result, c2, len);
		result[len] = '\0';
	} else {
		const size_t len1 = strlen(c1);
		const size_t len2 = strlen(c2);
		const size_t len = 1 + len1 + len2;
		
		result = (char*)malloc(sizeof(char)*(len+1));
		strncpy(result, c1, len1);
		result[len1] = ' ';
		strncpy(result+len1+1, c2, len2);
		result[len] = '\0';
		free(c1);
	}
	return result;
}




/* Main program entrance point*/
int main(int argc, char** argv) {
	// Parse program arguments
    for(int i=1;i<argc;i++) {
    	char* arg = argv[i];
    	if(strlen(arg) <= 0) continue;
    	
    	if(arg[0] == '-') {
			if(!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
				printHelp(argv[0]);
				return EXIT_SUCCESS;
			} else if(!strcmp(arg, "-9") || !strcmp(arg, "--kill")) {
				kill9 = true;
			} else if(!strcmp(arg, "-v") || !strcmp(arg, "--verbose")) {
				verbose = true;
			} else if(!strcmp(arg, "-d") || !strcmp(arg, "--daemon")) {
				daemonize = true;
			} else {
				fprintf(stderr, "Illegal argument: %s\n", arg);
				return EXIT_FAILURE;
			}
    	} else {
    		// No more options. Check if there are enough remaining arguments
    		// The basic syntax is ./program [OPTIONS] TIMEOUT PROGRAM [ARGS]
    		// Whereas timeout and program are obligatory
    		if (i+2 > argc) {		// Check if timeout and program are given
				fprintf(stderr, "Not enough arguments. Check %s --help, if you need help\n", argv[0]);
				if(i+1 > argc) {
					// Check if argument is number
					fprintf(stderr, "  Missing: TIMEOUT PROGRAM\n");
				} else {
					if(is_numeric(argv[i]))
						fprintf(stderr, "  Missing: PROGRAM\n");
					else
						fprintf(stderr, "  Missing: TIMEOUT\n");
				}
				return EXIT_FAILURE;
    		} else {
				int seconds = atoi(argv[i]);
				if(seconds < 0) {
					fprintf(stderr, "Timeout cannot be negative");
					return EXIT_FAILURE;
				}
				timeout = (unsigned int)seconds;
				
				// Merge program and optional program arguments 
				for(int j=i+1;j<argc;j++)
					command = strappend(command, argv[j]);
				
				break;
			}
    	}
    	
    }
    
    // Check program arguments
	if(command == NULL || strlen(command) <= 0) {
		fprintf(stderr, "Not enough arguments. Check %s --help, if you need help\n", argv[0]);
		fprintf(stderr, "  Missing: TIMEOUT PROGRAM\n");
		return EXIT_FAILURE;
	}
    
    // Fork daemon, if desired
    if (daemonize) {
    	fork_daemon();
    }
    
    
    // Fork and then execute command
    proc_pid = fork();
    runtime = -millis();
    if(proc_pid == 0) {
		// I am the child process. Execute command
		return system(command);
	} else {
		// Wait for pid to be executed
		int status;
		pid_t wait_status;
		
		// Signal handlers
		signal(SIGINT, sig_handler);
		signal(SIGTERM, sig_handler);
		signal(SIGALRM, sig_handler);
		
		if(verbose) printf("Child process forked with pid %d.\n", proc_pid);
		
		// Set alarm
		if(timeout > 0) alarm(timeout);
		wait_status = waitpid(proc_pid, &status, 0);		// Wait for child
		runtime += millis();
		if(wait_status < 0) {
			fprintf(stderr, "Error waiting for process: %s\n", strerror(errno));
			return EXIT_FAILURE;
		}
		status = WEXITSTATUS(status);		// Get real exit status
		if(status != 0) {
			if(verbose) fprintf(stderr, "Process exited with status %d after %ld milliseconds\n", status, runtime);
			return status;
		} else {
			if(verbose) printf("Process completed after %ld milliseconds\n", runtime);
			return status;
		}
	}
    
    return EXIT_SUCCESS;
}





static void fork_daemon(void) {
	pid_t pid = fork();
	if(pid < 0) {
		fprintf(stderr, "Fork daemon failed\n");
		exit(EXIT_FAILURE);
	} else if(pid > 0) {
		// Success. The parent leaves here
		exit(EXIT_SUCCESS);
	}
	
	/* Fork for the second time to detach from terminal */
	pid = fork();
	if(pid < 0) {
		fprintf(stderr, "Fork daemon failed (second fork)\n");
		exit(EXIT_FAILURE);
	} else if(pid > 0) {
		// Success. The parent again leaves here
		exit(EXIT_SUCCESS);
	}
}





/** Print help message */
static void printHelp(const char* progname) {
	printf("Usage: %s [OPTIONS] TIMEOUT PROGRAM\n", progname);
	printf("\n");
	printf("OPTIONS:\n");
	printf("\t-h  --help      Print this help message\n");
	printf("\t-9  --kill      Send SIGKILL instead of SIGTERM when timeout\n");
	printf("\t-v  --verbose   Verbosity on\n");
	printf("\t-d  --daemon    Run program as daemon\n");
	printf("\n");
	printf("  e.g. Execute the program cat with a timeout of 5 seconds:\n");
	printf("    %s 5 cat\n", progname);
	printf("\n");
	printf("2015, Felix Niederwanger, Version %s\n", VERSION);
}

static void terminate_process(void) {
	int sig;		// Signal
	
	if(proc_pid <= 0) return;
	else {
		if(kill9){
			printf("Killing");
			sig = SIGKILL;
		} else {
			printf("Terminating");
			sig = SIGTERM;
		}
		printf(" process %d ... ", proc_pid);
		kill(proc_pid, sig);
		printf("done\n");
	}
}



static void sig_handler(int sig_no) {
	switch(sig_no) {
		case SIGALRM:
			// Timeout
			printf("TIMEOUT after %ld milliseconds. ", runtime+millis());
			terminate_process();
			exit(EXIT_FAILURE);
			break;
		case SIGINT:
		case SIGTERM:
			if(proc_pid <= 0) exit(EXIT_FAILURE);
			printf("Program termination request\n");
			if(proc_pid > 0) kill(proc_pid, sig_no);
			exit(EXIT_FAILURE);
			return;
	}
}
