//Created by Illia Abdullaiev on 03/01/2018

#define _POSIX_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

int currentForegroundMode = 0;
int newForeGroundMode = 0;
int foregroundProcessID;
int foregoundExitSignal = 0;

void handleSIGNINT(int signal) {
    int exitStatus;

    if (foregroundProcessID != 0) {
        foregoundExitSignal = signal;
        kill(foregroundProcessID, SIGINT);
        waitpid(foregroundProcessID, &exitStatus, 0);
        foregroundProcessID = 0;
    }
}

void handleSIGTSTP(int signal) {
    if (currentForegroundMode == 0) {
        newForeGroundMode = 1;
    } else {
        newForeGroundMode = 0;
    }
}


int main(void) {
    int SIZE = 2048;
    int parentProcessID = getpid();
    char processIdStr[6];
    pid_t backgroundProcessIDs[2048];
    char foregroundProcessStatus[100];
    sprintf(foregroundProcessStatus, "no foreground processes have been run yet");

    //Initialize process arrays with zero values. It will show that the element by certain index has not been initialized.
    for (int i = 0; i < SIZE; i++) {
        backgroundProcessIDs[i] = 0;
    }

    //Convert current process ID int to a string for later usage.
    sprintf(processIdStr, "%d", parentProcessID);

    //Make sure smallsh is not terminated by SIGINT signal.
    struct sigaction SIGINT_action = {0};
    SIGINT_action.sa_handler = handleSIGNINT;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL);

    //Add handler SIGTSTP signal to enter/exit foreground-only mode.
    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = handleSIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = 0;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);


    //Begin asking for commands.
    while (1) {
        int isBackgroundMode = 0;

        //Check if foreground process was terminated by ctrl-c
        if (foregoundExitSignal != 0) {
            printf("terminated by signal %d\n", foregoundExitSignal);
            fflush(stdout);
            foregoundExitSignal = 0;
        }

        //Check if foreground mode has changed.
        if (currentForegroundMode != newForeGroundMode) {
            currentForegroundMode = newForeGroundMode;

            if (currentForegroundMode == 0) {
                printf("\nExiting foreground-only mode\n");
                fflush(stdout);
            } else {
                printf("\nEntering foreground-only mode (& is now ignored)\n");
                fflush(stdout);
            }
        }

        //Check for any completed background processes.
        for (int i = 0; i < SIZE; i++) {
            if (backgroundProcessIDs[i] != 0) {
                //Do not block execution of the parent process.
                int existStatus;
                int processCompleted = waitpid(backgroundProcessIDs[i], &existStatus, WNOHANG);
                if (processCompleted != 0) {
                    if (WIFEXITED(existStatus)) {
                        printf("background pid %d is done: exit value %d\n", backgroundProcessIDs[i],
                               WEXITSTATUS(existStatus));
                        backgroundProcessIDs[i] = 0;
                    } else if (WIFSIGNALED(existStatus)) {
                        printf("background pid %d is done: terminated by signal %d\n", backgroundProcessIDs[i],
                               WTERMSIG(existStatus));
                        backgroundProcessIDs[i] = 0;
                    }
                }
            }
        }

        //Ask for input.
        printf(": ");
        fflush(stdout);

        //Read the input command into a string of 2048 characters max.
        char input[SIZE + 6];
        char *result = fgets(input, SIZE, stdin);

        //If fgets was interrupted by a signal, remove the error status from stdin and ask the user for input, again.
        if (result == NULL) {
            clearerr(stdin);
            continue;
        }

        //Replace $$ with process ID.
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

        //Ignore it if it begins with # or empty.
        if (input[0] == '#' || input[0] == '\n') {
            fflush(stdout);
            continue;
        }

        //Make an array of pointers to command words.
        char *command[SIZE];
        char delimiter[] = " ";
        int count = 0;
        command[count] = strtok(input, delimiter);

        while (command[count] != NULL) {
            command[++count] = strtok(NULL, delimiter);
        }

        //Remove \n from the last word in the command array.
        command[count - 1] = strtok(command[count - 1], "\n");

        //Check if the command is to be run in the background mode.
        if (strcmp(command[count - 1], "&") == 0) {
            //A command can be run in background only if smallsh is not in foreground mode.
            if (currentForegroundMode == 0) {
                isBackgroundMode = 1;
            }

            //Remove & from the end of the command list.
            command[count - 1] = NULL;
        }


        //Handle the commands supported byt smallsh: cd, status, exit.
        if (strcmp(command[0], "cd") == 0) {
            const char *directory;

            if (command[1] == NULL) {
                //Go to HOME environment variable if no argument.
                directory = getenv("HOME");
            } else {
                //Respect the first argument otherwise.
                directory = command[1];
            }

            int result = chdir(directory);
            if (result == -1) {
                printf("%s: no such file or directory\n", directory);
                fflush(stdout);
            }
            continue;
        }

        //Handle status command: print out the exit status of the last foreground process.
        if (strcmp(command[0], "status") == 0) {
            //Prints out either the exit status or the terminating signal of the last foreground process
            printf("%s\n", foregroundProcessStatus);
            fflush(stdout);
            continue;
        }

        //Handle exit command: kill all background processes and exit smallsh.
        if (strcmp(command[0], "exit") == 0) {
            for (int i = 0; i < SIZE; i++) {
                if (backgroundProcessIDs[i] != 0) {
                    kill(backgroundProcessIDs[i], SIGTERM);
                }
            }
            exit(0);
        }

        //If command is not supported by smallsh, fork a new process to try to execute that command.
        pid_t forkProcessID = fork();

        switch (forkProcessID) {
            case -1: {
                printf("could not spawn a process!\n");
                fflush(stdout);
                exit(1);
            }
            case 0: {
                //This is a child process. Handle redirections.
                int redirectingStdin = 0;
                int redirectingStdout = 0;
                int redirectError = 0;

                //Handle stdout redirection if needed. The output file should be opened for writing only.
                count = 0;
                while (command[count] != NULL) {
                    //stdout redirection.
                    if (strcmp(command[count], ">") == 0) {
                        redirectingStdout = 1;
                        int customOut = open(command[count + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (customOut == -1) {
                            printf("cannot open %s for output\n", command[count + 1]);
                            fflush(stdout);
                            exit(1);
                        }
                        int result = dup2(customOut, 1);
                        if (result == -1) {
                            printf("cannot open %s for output\n", command[count + 1]);
                            fflush(stdout);
                            exit(1);
                        }

                        //remove stdout redirection from command array
                        command[count] = NULL;
                        command[count + 1] = NULL;
                        while (command[count + 2] != 0) {
                            command[count] = command[count + 2];
                            count++;
                        }
                        break;
                    }
                    count++;
                }

                //Handle stdin redirection. The input file should be open for reading only.
                count = 0;
                while (command[count] != NULL) {
                    //stdin redirection.
                    if (strcmp(command[count], "<") == 0) {
                        redirectingStdin = 1;

                        int customIn = open(command[count + 1], O_RDONLY);
                        if (customIn == -1) {
                            printf("cannot open %s for input\n", command[count + 1]);
                            fflush(stdout);
                            exit(1);
                        }
                        int result = dup2(customIn, 0);
                        if (result == -1) {
                            printf("cannot open %s for input\n", command[count + 1]);
                            fflush(stdout);
                            exit(1);
                        }

                        //Remove stdin redirection from command array.
                        command[count] = NULL;
                        command[count + 1] = NULL;
                        while (command[count + 2] != 0) {
                            command[count] = command[count + 2];
                            count++;
                        }
                        break;
                    }
                    count++;
                }

                //If the program is to be run in the background mode, redirect stdin and stdout to /dev/null unless redirections are provided.
                int devNull;
                if (redirectingStdout == 0 && isBackgroundMode == 1) {
                    devNull = open("/dev/null", O_WRONLY);
                    dup2(devNull, 1);
                }
                if (redirectingStdin == 0 && isBackgroundMode == 1) {
                    devNull = open("/dev/null", O_RDONLY);
                    dup2(devNull, 0);
                }

                //Ignore SIGINT signal is the child is running in background mode.
                //Only foreground processes should be killed by SIGINT.
                struct sigaction SIGINT_action = {0};

                if (isBackgroundMode) {
                    SIGINT_action.sa_handler = SIG_IGN;
                } else {
                    SIGINT_action.sa_handler = SIG_DFL;
                }

                sigfillset(&SIGINT_action.sa_mask);
                SIGINT_action.sa_flags = 0;
                sigaction(SIGINT, &SIGINT_action, NULL);

                execvp(command[0], command);

                //execvp failed if code reached the statements below. Set exit value to 1.
                printf("%s: no such file or directory\n", command[0]);
                fflush(stdout);
                exit(1);
            }
            default: {
                //This is a parent process.

                if (isBackgroundMode) {
                    //Print process ID of the new background process.
                    printf("background pid is %d\n", forkProcessID);
                    fflush(stdout);

                    //Save process ID for later usage. When this process finished, the user will be notified of it.
                    //If the shell exits, it will use this ID to kill the process.
                    int countP = 0;
                    while (backgroundProcessIDs[countP] != 0) {
                        countP++;
                    }
                    backgroundProcessIDs[countP] = forkProcessID;
                } else {
                    //Wait until child foreground process completes
                    foregroundProcessID = forkProcessID;
                    int exitStatus;
                    waitpid(forkProcessID, &exitStatus, 0);
                    foregroundProcessID = 0;

                    //Save exit status or termination signal for smallsh built-in status command
                    if (WIFEXITED(exitStatus)) {
                        sprintf(foregroundProcessStatus, "exit value %d", WEXITSTATUS(exitStatus));
                    } else if (WIFSIGNALED(exitStatus)) {
                        sprintf(foregroundProcessStatus, "terminated by signal %d", WTERMSIG(exitStatus));
                    }
                }
            }
        }
    }
}
