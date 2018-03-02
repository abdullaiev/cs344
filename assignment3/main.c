//Created by Illia Abdullaiev on 03/01/2018

#define _POSIX_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
    int SIZE = 2048;
    pid_t childProcesses[2048];
    int parentProcessID = getpid();
    int childProcessCount = 0;
    char processIdStr[6];

    //Replace $$ with process ID
    sprintf(processIdStr, "%d", parentProcessID);

    //Begin asking for commands
    while (1) {
        //Print a message about completed background process. (waitpd ?)
        //Print the number of the signal that killed a child foreground process.

        //Ask for input
        printf(": ");
        fflush(stdout);

        //Read the input command into a string of 2048 characters max
        char input[SIZE+6];
        fgets(input, SIZE, stdin);

        //Replace $$ with process ID
        char *replace = "$$";
        char *position = strstr(input, replace);
        char buffer[SIZE];
        memset(buffer, '\0', strlen(buffer));
        if (position) {
            if (input == position) {
                strcpy(buffer, processIdStr);
                strcat(buffer, position + strlen(replace));
            } else {
                strncpy(buffer, input, strlen(input) - strlen(position));
                strcat(buffer, processIdStr);
                strcat(buffer, position + strlen(replace));
            }
            memset(input, '\0', strlen(input));
            strcpy(input, buffer);
        }

        //Ignore it if it begins with # or empty
        if (input[0] == '#' || input[0] == '\n') {
            fflush(stdout);
            continue;
        }

        //Make an array of pointers to command words
        char* command[SIZE];
        char delimiter[] = " ";
        int count = 0;
        command[count] = strtok(input, delimiter);

        while (command[count] != NULL) {
            command[++count] = strtok(NULL, delimiter);
        }

        //Remove \n from the last word in the command array
        command[count-1] = strtok(command[count-1], "\n");


        //Handle the commands supported byt smallsh: cd, status, exit
        if (strcmp(command[0], "cd") == 0) {
            const char *directory;

            if (command[2] == NULL) {
                //goes to HOME environment variable if no argument
                directory = getenv("HOME");
            } else {
                //respects the first argument otherwise, supports absolute and relative path
                directory = command[2];
            }

            int result = chdir(directory);
            if (result == -1) {
                printf("\n%s: no such file or directory\n", directory);
                fflush(stdout);
            }
            continue;
        }

        if (strcmp(command[0], "status") == 0) {
            //status prints out either the exit status or the terminating signal of the last foreground process
            // (not both, processes killed by signals do not have exit statuses!) ran by your shell.
            //todo
            continue;
        }

        if (strcmp(command[0], "exit") == 0) {
            //kill all process that have been started
            for(int k = 0; k < childProcessCount; k++) {
                kill(childProcesses[k], SIGTERM);
            }
            exit(0);
        }

        //If command is not supported bu smallsh, fork a new process to try to execute that command
        int bogus = -100;
        pid_t forkProcessID = bogus;
        int exitStatus = bogus;
        int exitMethod = bogus;

        forkProcessID = fork();

        switch (forkProcessID) {
            case -1: {
                printf("Could not spawn a process!\n");
                fflush(stdout);
                exit(1);
            }
            case 0: {
                //This is a child process

                //Handle redirections (dup2())
                //If the program is to be run in the background mode, redirect stdin and stdout to /dev/null unless redirections are provided
                //Remove redirections from the array
                //The output file should be opened (created) for writing only
                //The input file should be created for reading only.

                execvp(command[0], command);

                //If file creation failed, print an error message.
                printf("%s: no such file or directory\n", command[0]);
                fflush(stdout);
                exit(1);
            }
            default: {
                //this is parent
                //wait until completion of the child process
                printf("Parent waiting................\n");
                fflush(stdout);
//                waitpid(forkProcessID, &exitMethod, 0);
                wait(&exitMethod);
                printf("Parent waited until completion\n");
                fflush(stdout);
            }

        }

        //If & is the last char, run the command in background mode.
            //Remove & if it is the last word
        //If background process, print process ID of the new background process.

        //Handle ctrl-c SIGNT signal. Make sure that only foreground process is terminated and shell still executes (sigaction)
        //Handle ctrl-z SIGTSTP signal. todo
    }
}
