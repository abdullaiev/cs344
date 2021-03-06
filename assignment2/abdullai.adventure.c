//Created by Illia Abdullaiev on 02/11/2018

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <pthread.h>

struct room {
    char name[256];
    char connections[7][256];
    char type[256];
};

//Finds the most recently created folder with room files and stores it in the passed string.
int getMostRecentDirectoryName(char name[]) {
    DIR *currentDirectory = opendir(".");
    time_t timestamp = 0;
    struct dirent *dir;
    struct stat directoryStat;

    if (!currentDirectory) {
        printf("Could not read the current directory to parse rooms data!\n");
        return 1;
    }

    //Loop over all the folders in the current directory.
    while ((dir = readdir(currentDirectory)) != NULL) {
        //If it is a directory that has "abdullai.rooms." in its name.
        if (dir->d_type == DT_DIR && strstr(dir->d_name, "abdullai.rooms.") != NULL) {
            //Get the stats on this directory.
            stat(dir->d_name, &directoryStat);

            //Compare if this one is the most recently updated one.
            if (directoryStat.st_mtime > timestamp) {
                //If it is, save its name.
                timestamp = directoryStat.st_mtime;
                snprintf(name, 256, "%s", dir->d_name);
            }
        }
    }

    closedir(currentDirectory);
    return 0;
}

//Reads given room file line by line, storing room details such as name, type and connections in a room struct by given index.
int readByLine(char fileName[], struct room rooms[], int roomIndex) {
    FILE *filePointer = fopen(fileName, "r");
    char *line = NULL;
    int lineCount = 0;
    int connectionCount = 0;
    size_t len = 0;
    char *token;

    //Foe every line in the room file.
    while (getline(&line, &len, filePointer) != -1) {
        if (lineCount == 0) {
            //Save room name if this is the first line of the file.
            token = strtok(line, "ROOM NAME: ");
            //Remove \n from room name.
            strtok(token, "\n");
            //Save room name to the array of room structs.
            snprintf(rooms[roomIndex].name, 256, "%s", token);
        } else if (strstr(line, "CONNECTION") != NULL) {
            //Save connection room name into struct connections array.
            token = strtok(line, " ");
            token = strtok(NULL, " ");
            token = strtok(NULL, " ");
            strtok(token, "\n");
            //Token has room name now.
            snprintf(rooms[roomIndex].connections[connectionCount], 256, "%s", token);
            connectionCount++;
        } else {
            //Save room type to the array of room structs.
            if (strstr(line, "START_ROOM") != NULL) {
                snprintf(rooms[roomIndex].type, 256, "START_ROOM");
            } else if (strstr(line, "MID_ROOM") != NULL) {
                snprintf(rooms[roomIndex].type, 256, "MID_ROOM");
            } else if (strstr(line, "END_ROOM") != NULL) {
                snprintf(rooms[roomIndex].type, 256, "END_ROOM");
            }

            //Since room type is the last piece of information we need, terminate the loop to avoid going over empty lines.
            //* denotes the end of connections list.
            snprintf(rooms[roomIndex].connections[connectionCount], 256, "*");
            break;
        }

        lineCount++;
    }

    fclose(filePointer);

    if (line) {
        free(line);
    }

    return 0;
}

//Parses each file in the given room files directory for further processing.
int parseRoomFiles(char directory[], struct room rooms[]) {
    DIR *roomFilesDirectory = opendir(directory);
    struct dirent *file;

    if (!roomFilesDirectory) {
        printf("Could not read the directory with room files!\n");
        return 1;
    }

    int count = 0;

    //For every room file in the given directory.
    while ((file = readdir(roomFilesDirectory)) != NULL) {
        char filePath[256];

        //Make sure it is a room file with room details.
        if (file->d_type != DT_REG || strstr(file->d_name, "_room") == NULL) {
            continue;
        }

        //Get the file name and parse each line of this file with readByLine function.
        snprintf(filePath, sizeof filePath, "%s/%s", directory, file->d_name);
        readByLine(filePath, rooms, count);
        count++;
    }

    closedir(roomFilesDirectory);
    return 0;
}

//Finds the room by given query and type criteria.
int findRoom(struct room rooms[], char query[], char type[]) {
    int i;
    int length = 7;

    //Loop over all the rooms.
    for (i = 0; i < length; i++) {
        if (strcmp(type, "name") == 0) {
            if (strcmp(rooms[i].name, query) == 0) {
                //Found the room with name == query.
                return i;
            }
        } else if (strcmp(type, "type") == 0) {
            if (strcmp(rooms[i].type, query) == 0) {
                //Found the room with type == query.
                return i;
            }
        }
    }

    return -1;
}

//Prints out current possible connections to other room and stores user's input in the passed input string.
void askForInput(struct room rooms[], int currentRoom, char input[], int askedTime) {
    //In regular mode, game outputs current location and possible connections.
    //If user asked for current time, the game will output current time and then skip current location and possible connections.
    if (askedTime == 0) {
        printf("CURRENT LOCATION: %s\n", rooms[currentRoom].name);
        printf("POSSIBLE CONNECTIONS: ");

        int i = 0;
        while (rooms[currentRoom].connections[i][0] != '*') {
            if (i != 0) {
                printf(", ");
            }
            printf("%s", rooms[currentRoom].connections[i]);
            i++;
        }
        printf(".\n");
    }

    printf("WHERE TO? >");
    fgets(input, 256, stdin);

    //Remove the new line symbol.
    size_t len = strlen(input) - 1;
    if (input[len] == '\n') {
        input[len] = '\0';
    }

    printf("\n");
}

//Checks if there is such connection in the current room that matches to the given input.
int isInputValid(struct room rooms[], int currentRoom, char input[]) {
    int i = 0;
    while (rooms[currentRoom].connections[i][0] != '*') {
        if (strcmp(rooms[currentRoom].connections[i], input) == 0) {
            return 1;
        }
        i++;
    }
    printf("HUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN.\n\n");
    return 0;
}

//End the game.
void congratulateUser(char path[][256], int steps) {
    printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", steps);

    int i = 0;
    //Print out every room that the user has visited.
    while (path[i][0] != '*') {
        printf("%s\n", path[i]);
        i++;
    }
}


//Gets current user's time, formats it as "1:03pm, Tuesday, September 13, 2016" and stores into file currentTime.txt.
//This functions is executed in a separate thread. See startTimeThread for reference.
void* writeCurrentTime(void* arg) {
    //Acquire a mutex look for this thread so no other threads can lock it.
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&mutex);

    char timeStr[256];
    struct tm *tmStruct;
    time_t currentTime;
    currentTime = time(NULL);
    tmStruct = localtime(&currentTime);
    strftime(timeStr, sizeof(timeStr), "%I:%M%p, %A, %B %d, %Y", tmStruct);

    //Save parsed time into the file.
    FILE *filePointer = fopen("currentTime.txt", "w");
    fprintf(filePointer, "%s", timeStr);
    fclose(filePointer);

    //Current time has been stored in a file, the mutex lock can be released now.
    pthread_mutex_unlock(&mutex);
    return NULL;
}

//Starts a separate thread to write current time into a file.
void startTimeThread() {
    pthread_t timeThreadID;
    pthread_create(&timeThreadID, NULL, writeCurrentTime, NULL);
    pthread_join(timeThreadID, NULL);
}

//Reads time stored in currentTime.txt and prints it to the console.
void readCurrentTime() {
    char *line = NULL;
    size_t len = 0;
    FILE *filePointer = fopen("currentTime.txt", "r");
    //There will be just one line with time details. No need to loop over file contents.
    getline(&line, &len, filePointer);
    printf("%s\n\n", line);
    fclose(filePointer);
}

//Starts asking the user for input and ends the game once the end-room is reached.
void startGame(struct room rooms[]) {
    int steps = 0;
    int currentRoom = findRoom(rooms, "START_ROOM", "type"); //Begin from start room.
    int endRoom = findRoom(rooms, "END_ROOM", "type"); //Store end room index for later usage.
    char path[100][256]; //An array to store all rooms visited.
    char input[256]; //User's input.
    int askedTime = 0; //When set to 1, current room and possible connections will not be logged out.

    //Keep asking for room to enter until end room is reached.
    while (1) {
        askForInput(rooms, currentRoom, input, askedTime);
        askedTime = 0;

        //Check if the user wishes to see current time.
        if (strcmp(input, "time") == 0) {
            askedTime = 1;
            //Store time in a separate thread. That thread will use mutexes. See writeCurrentTime();
            startTimeThread();
            readCurrentTime();
            continue;
        }

        //Validate the input otherwise.
        int isValid = isInputValid(rooms, currentRoom, input);

        //If input is not valid, do not update steps taken and the history of rooms visited.
        if (isValid == 0) {
            continue;
        }

        currentRoom = findRoom(rooms, input, "name");
        //Save this room to the path taken.
        snprintf(path[steps], 256, "%s", input);

        //Update steps count.
        steps++;

        //Check if this is the end room.
        if (currentRoom == endRoom) {
            //Use an asterisk as the end of path delimiter.
            snprintf(path[steps], 256, "*");
            congratulateUser(path, steps);
            break;
        }
    }
}

int main(void) {
    //Find the most recent folder.
    char directoryName[256];
    getMostRecentDirectoryName(directoryName);

    //Read data from room files and store it into the program.
    struct room rooms[7];
    parseRoomFiles(directoryName, rooms);

    //Start asking for input.
    startGame(rooms);

    exit(0);
}