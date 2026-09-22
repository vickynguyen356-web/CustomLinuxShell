#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>   // to use wait()
#include <string.h>
#include <errno.h>
#include <fcntl.h>      // for open() and creat()
#include <sys/stat.h>

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define BUFFER_SIZE 50
#define HISTORY_SIZE 10 // for 10 most recent commands


static char buffer[BUFFER_SIZE];
static char history[HISTORY_SIZE][MAX_LINE];
static int history_count = 0;

/* declarations of history file functions */
void save_history();
void load_history();
void add_to_history(const char *entry);

/**
 * setup() reads in the next command line, separating it into distinct tokens
 * using whitespace as delimiters. setup() sets the args parameter as a
 * null-terminated string.
 */
void setup(char inputBuffer[], char *args[],int *background, char cmdArgs[])
{
    int length, /* # of characters in the command line */
        i,      /* loop index for accessing inputBuffer array */
        start,  /* index where beginning of next command parameter is */
        ct;     /* index of where to place the next parameter into args[] */

    ct = 0;

    // check for ctrl c command
    do {
	    length = read(STDIN_FILENO, inputBuffer, MAX_LINE);
    } while  (length == -1 && errno == EINTR);
   
   strncpy(cmdArgs, inputBuffer, MAX_LINE);
   cmdArgs[MAX_LINE-1] = '\0';
   // remove any trailing newline from cmdArgs
   cmdArgs[strcspn(cmdArgs, "\n")] = '\0'; 

    start = -1;
    if (length == 0) {
	save_history();
        exit(0);            /* ^d was entered, end of user command stream */
    }
    if (length < 0){
        perror("error reading the command");
    exit(-1);           /* terminate with error code of -1 */
    }

    /* examine every character in the inputBuffer */
    for (i = 0; i < length; i++) {
        switch (inputBuffer[i]){
        case ' ':
        case '\t' :               /* argument separators */
            if(start != -1){
                args[ct] = &inputBuffer[start];    /* set up pointer */
                ct++;
            }
            inputBuffer[i] = '\0'; /* add a null char; make a C string */
            start = -1;
            break;

        case '\n':                 /* should be the final char examined */
            if (start != -1){
                args[ct] = &inputBuffer[start];
                ct++;
            }
            inputBuffer[i] = '\0';
            args[ct] = NULL; /* no more arguments to this command */
            break;

        case '&':
            *background = 1;
            inputBuffer[i] = '\0';
            break;

        default :             /* some other character */
            if (start == -1)
                start = i;
        }
    }
    args[ct] = NULL; /* just in case the input line was > 80 */
}

/* signal handler function*/
void handle_SIGINT() {
    char output[BUFFER_SIZE * HISTORY_SIZE];
    int len = 0;
    int total = history_count;
    int start;
    /* only want most recent 10 commands */
    if (total > HISTORY_SIZE) {
        start = total - HISTORY_SIZE;
    } else {
        /* there are 10 or less recent commands */
        start = 0;
    }

    for (int i = start; i < total; i++) {
        // get correct index
        int j = i % HISTORY_SIZE;
        char line[MAX_LINE + 20]; // commands with args can exceed 50 chars
        // get # of characters the line that's printed will be and writes into line[]
        int n = snprintf(line, sizeof(line), "%d %s\n", i+1, history[j]);
        write(STDOUT_FILENO, line, n);
    }

    write(STDOUT_FILENO, "COMMAND->", 9);
}

/* loading history from history file */
void load_history() {
    int fd = open("nguyen-3068.history", O_RDONLY);
    if (fd == -1) {
        // file doesn't exist
        history_count = 0;
        return;
    }

    off_t size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    if (size == 0) {
        close(fd);
        exit(-1);
    }

    char *raw = malloc(size + 1);
    if (raw == NULL) {
        close(fd);
        perror("Error allocating memory for history");
        exit(-1);
    }

    read(fd, raw, size);
    raw[size] = '\0';
    close(fd);

    /* parsing lines into history */
    char *line_start = raw;
    for (int i = 0; i < size && history_count < HISTORY_SIZE; i++) {
        if (raw[i] == '\n') {
           int len = &raw[i] - line_start;
           if (len > 0) {
            strncpy(history[history_count], line_start, MAX_LINE - 1);
            history[history_count][MAX_LINE - 1] = '\0';
	    // removing any trailing \r or \n
	    history[history_count][strcspn(history[history_count], "\r\n")] = '\0';
            history_count++;
           }
           line_start = &raw[i + 1];
        }
    }

    free(raw);
}

/* save history to file */
void save_history() {
    int fd = open("nguyen-3068.history", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Error opening history file for writing");
        exit(-1);
    }

    for (int i = 0; i < history_count; i++) {
        write(fd, history[i], strlen(history[i]));
        write(fd, "\n", 1);
    }

    close(fd);
}

/* adds to history */
void add_to_history(const char *entry) {
	// skipping empty/white-space entries
   if (entry == NULL || strlen(entry) == 0) return;
    if (history_count < HISTORY_SIZE) {
        // append to history
        strncpy(history[history_count], entry, MAX_LINE - 1);
        history[history_count][MAX_LINE - 1] = '\0';
        history_count++;
    } else {
        // shift every command up and drop the oldest
        for (int i = 0; i < HISTORY_SIZE - 1; i++) {
            strncpy(history[i], history[i + 1], MAX_LINE - 1);
            history[i][MAX_LINE - 1] = '\0';
        }
        // add the new command to the end
        strncpy(history[HISTORY_SIZE - 1], entry, MAX_LINE - 1);
        history[HISTORY_SIZE - 1][MAX_LINE - 1] = '\0';
    }
}

int main(void) {
    char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
    int background;             /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2+1];   /* 80-character command line has 40 args max */
    /* set up signal handler */
    struct sigaction handler;
    handler.sa_handler = handle_SIGINT;
    sigaction(SIGINT, &handler, NULL);
    char target[MAX_LINE];
    char exec_buffer[MAX_LINE];

    /* checking if history file exists, if it does, loads history. if it doesn't, keep buffer empty */
    load_history();
   

    while (1){            /* Program terminates normally inside setup */
    background = 0;
    printf("COMMAND->");
        fflush(0);

	char cmdArgs[MAX_LINE]; // stores argument of command

        setup(inputBuffer, args, &background, cmdArgs);       /* get next command */

	/* handling exit */
	if (args[0] != NULL && strcmp(args[0], "exit") == 0) {
		save_history();
		exit(0);
	}

	/* adding an exit command since ctrl C is used to display recent commands */
	if (args[0] != NULL && strcmp(args[0], "exit") == 0) {
		save_history();
		exit(0);
	}

        /* handle r command */
        if (args[0] != NULL && strcmp(args[0], "r") == 0) {

            if (args[1] != NULL) {
                // get most recent command starting with args[1][0]
                char letter = args[1][0];
                int found = -1;
                int total = history_count;
                int start;
                /* only want most recent 10 commands */
                if (total > HISTORY_SIZE) {
                    start = total - HISTORY_SIZE;
                } else {
                    start = 0;
                }

                for (int i = total -1; i >= start; i--) {
                    if (history[i % HISTORY_SIZE][0] == letter) {
                        found = i;
                        break;
                    }
                }

                if (found == -1) {
                    printf("No command starting with '%c' found\n", letter);
                    continue;
                }
                strncpy(target, history[found % HISTORY_SIZE], MAX_LINE);  
            } else {
                // just repeat most current command
                if (history_count == 0) {
                    printf("No commands in history\n");
                    continue;
                }
                strncpy(target, history[(history_count - 1) % HISTORY_SIZE], MAX_LINE);
            }

            // echoing command
            printf("%s\n", target);
	    add_to_history(target);

	    
	   strncpy(exec_buffer, target, MAX_LINE); // copy before strtok modifies
	   exec_buffer[strcspn(exec_buffer, "\n")] = '\0';

           char *token = strtok(exec_buffer, " \t");
	   int ct = 0;
	   while (token != NULL) {
		   args[ct++] = token;
		   token = strtok(NULL, " \t");
	   }
	   args[ct] = NULL;
        } else {
		add_to_history(cmdArgs);
	}

        pid_t child;
        // child is forked to execute next command
        child = fork();
        if (child == 0) {
            // child invokes execvp
            execvp(args[0], args);
            exit(0);
        } else if (child > 0) {
            // parent process, checking background
            if (background == 0) {
                // parent must wait, & was in command
                waitpid(child, NULL, 0);
            } 
        } else {
            // error case
            printf("Command failed.");
            exit (-1);
        }

    }
}
