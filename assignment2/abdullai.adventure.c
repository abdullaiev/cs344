//Created by Illia Abdullaiev on 02/11/2018

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

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

    while ((dir = readdir(currentDirectory)) != NULL) {
        //If it is a directory that has "abdullai.rooms." in its name.
        if (dir->d_type == DT_DIR && strstr(dir->d_name, "abdullai.rooms.") != NULL) {
            stat(dir->d_name, &directoryStat);

            if (directoryStat.st_mtime > timestamp) {
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
    while ((file = readdir(roomFilesDirectory)) != NULL) {
        char filePath[256];

        //Make sure it is a room file with room details.
        if (file->d_type != DT_REG || strstr(file->d_name, "_room") == NULL) {
            continue;
        }

        snprintf(filePath, sizeof filePath, "%s/%s", directory, file->d_name);
        readByLine(filePath, rooms, count);
        count++;
    }

    closedir(roomFilesDirectory);
    return 0;
}

//Finds the room by given query and type criteria.
int findRoom(struct room rooms[], char query[], char type[]) {
    int i = 0;
    int length = 7;

    for (; i < length; i++) {
        if (strcmp(type, "name") == 0) {
            if (strcmp(rooms[i].name, query) == 0) {
                return i;
            }
        } else if (strcmp(type, "type") == 0) {
            if (strcmp(rooms[i].type, query) == 0) {
                return i;
            }
        }
    }

    return -1;
}

//Prints out current possible connections to other room and stores user's input in the passed input string.
void askForInput(struct room rooms[], int currentRoom, char input[]) {
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
    printf(".\nWHERE TO? >");
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
    while (path[i][0] != '*') {
        printf("%s\n", path[i]);
        i++;
    }
}

//Starts asking the user for input and ends the game once the end-room is reached.
void startGame(struct room rooms[]) {
    int steps = 0;
    int currentRoom = findRoom(rooms, "START_ROOM", "type");
    int endRoom = findRoom(rooms, "END_ROOM", "type");
    char path[1000][256];
    char input[256];

    while (1) {
        askForInput(rooms, currentRoom, input);

        if (isInputValid(rooms, currentRoom, input) == 1) {
            currentRoom = findRoom(rooms, input, "name");
            //Save this room to the path taken.
            snprintf(path[steps], 256, "%s", input);

            //Update steps count.
            steps++;

            //Check if this is the end room.
            if (currentRoom == endRoom) {
                snprintf(path[steps], 256, "*");
                congratulateUser(path, steps);
                break;
            }
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