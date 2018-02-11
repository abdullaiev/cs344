//Created by Illia Abdullaiev on 02/11/2018

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

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
                snprintf(name, "%s", dir->d_name);
            }
        }
    }

    closedir(currentDirectory);
    return 0;
}

int readByLine(char fileName[], char rooms[][10][256], int roomIndex) {
    FILE * filePointer = fopen(fileName, "r");
    char * line = NULL;
    int lineCount = 0;
    size_t len = 0;

    while (getline(&line, &len, filePointer) != -1) {
        snprintf(rooms[roomIndex][lineCount], "%s", line);
        lineCount++;
    }

    fclose(filePointer);

    if (line) {
        free(line);
    }

    return 0;
}

int parseRoomFiles(char directory[], char rooms[][10][256]) {
    DIR *roomFilesDirectory = opendir(directory);
    struct dirent *file;

    if (!roomFilesDirectory) {
        printf("Could not read the directory with room files!\n");
        return 1;
    }

    int count = 0;
    while ((file = readdir(roomFilesDirectory)) != NULL) {
        char filePath[256];

        //Make sure it is a .txt file with room details.
        if (file->d_type != DT_REG || strstr(file->d_name, ".txt") == NULL) {
            continue;
        }

        snprintf(filePath, sizeof filePath, "%s/%s", directory, file->d_name);
        readByLine(filePath, rooms, count);

        count++;
    }

    closedir(roomFilesDirectory);
}

void startGame(char rooms[][10][256]) {
    int steps = 0;
    int currentRoomIndex = 0;
}

int main(void) {
    //Find the most recent folder.
    char directoryName[256];
    getMostRecentDirectoryName(directoryName);

    //Read data from room files and store it into the program.
    char rooms[7][10][256];
    parseRoomFiles(directoryName, rooms);

    //Find the start room and update current room index.
    //Start asking for input.
    startGame(rooms);
}