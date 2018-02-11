//Created by Illia Abdullaiev on 02/10/2018

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

//Attaches processID to a hardcoded directory name.
void getDirectoryName(char name[]) {
    int processID = getpid();
    sprintf(name, "abdullai.rooms.%d", processID);
}

//Create a directory <STUDENT ONID USERNAME>.rooms.<PROCESS ID>"
int createDirectory() {
    char directoryName[30];
    getDirectoryName(directoryName);
    int result = mkdir(directoryName, 0755);
    if (result != 0) {
        printf("Error occurred while creating a directory for rooms.\n");
    }

    return result;
}

//Returns a random number in range min...max.
int getRandomNumberInRange(int min, int max) {
    return rand() % (max + 1 - min) + min;
}

//Assigns random room indices to an array of rooms. Makes sure there are no duplicates.
void getRandomRooms(int arr[], int NUMBER) {
    int count = 0;
    int random;
    int contains = 0;
    int i;

    while (count < NUMBER) {
        random = getRandomNumberInRange(0, 9);

        for (i = 0; i < NUMBER; i++) {
            if (arr[i] == random) {
                contains = 1;
                break;
            }
        }

        if (contains == 0) {
            arr[count] = random;
            count++;
        }

        contains = 0;
    }
}

//Initializes connections array with -1 for every element.
//-1 serves as an indicator of empty connection.
void initializeConnections(int connections[][6], int length) {
    int i;
    int j;

    for (i = 0; i < length; i++) {
        for (j = 0; j < length - 1; j++) {
            connections[i][j] = -1;
        }

        j = 0;
    }
}

//Checks that every room has the required minimum of connections.
int connectionsReady(int connections[][6], int length, int min) {
    int count = 0;
    int i;
    int j;

    for (i = 0; i < length; i++) {
        for (j = 0; j < length - 1; j++) {
            //Increment count as long as there is a connection (not -1 value)
            if (connections[i][j] != -1) {
                count++;
            } else {
                break;
            }
        }

        //If at least one room has less than required minimum of connections, the rooms are not ready yet.
        if (count < min) {
            return 0;
        }

        count = 0;
        j = 0;
    }

    return 1;
}

//Returns a number of connections in a room by given index
int numberOfConnections(int room[], int max) {
    int i = 0;
    for (; i < max; i++) {
        if (room[i] == -1) {
            return i;
        }
    }
    return i;
}

//Checks if a room has a certain connection.
int connectionExists(int room[], int max, int connection) {
    int i = 0;

    for (; i < max; i++) {
        if (room[i] == connection) {
            return 1;
        }
    }

    return 0;
}

void addConnection(int room[], int max, int connection) {
    int i = 0;

    for (; i < max; i++) {
        if (room[i] == -1) {
            room[i] = connection;
            break;
        }
    }
}

//Generate connections for each room in a way that the room has at least three at most six connections to other rooms.
//Make sure the connections within one room do not repeat and that a room is not pointing to itself.
void generateConnections(int connections[][6], int rooms, int min) {
    int max = rooms - 1;

    while (connectionsReady(connections, rooms, 3) != 1) {
        int room1 = getRandomNumberInRange(0, max);
        int room2 = getRandomNumberInRange(0, max);

        //1. Check that two random rooms are not pointing to itself.
        if (room1 == room2) {
            continue;
        }

        //2. Check that room1 number of connections is not max
        if (numberOfConnections(connections[room1], max) >= max) {
            continue;
        }

        //3. Check that room2 number of connections is not max
        if (numberOfConnections(connections[room2], max) >= max) {
            continue;
        }

        //4. Check that room1 is not already pointing to room2
        if (connectionExists(connections[room1], max, room2) == 1) {
            continue;
        }

        //5. Check that room2 is not already pointing to room1
        if (connectionExists(connections[room2], max, room1) == 1) {
            continue;
        }

        //6. If everything above checks, the connections can be added.
        addConnection(connections[room1], max, room2);
        addConnection(connections[room2], max, room1);
    }
}

//Writes the connections of each room into its own file.
void writeConnections(const char **allRooms, int randomRooms[], int connections[][6], int roomsNumber) {
    char extension[] = {".txt"};
    char directoryName[30];
    getDirectoryName(directoryName);

    int i = 0;
    int j = 0;

    for (; i < roomsNumber; i++) {
        //construct file path
        const char *roomName = allRooms[randomRooms[i]];
        char fileName[256];
        snprintf(fileName, sizeof fileName, "%s/%s%s", directoryName, roomName, extension);

        FILE *roomFile;
        roomFile = fopen(fileName, "w");
        if (roomFile == NULL) {
            printf("Error creating the file for room %s", roomName);
        }

        fprintf (roomFile, "ROOM NAME: %s\n", roomName);

        const char *connection;
        for (; j < roomsNumber - 1; j++) {
            if (connections[i][j] == -1) {
                break;
            }

            connection = allRooms[randomRooms[connections[i][j]]];
            fprintf (roomFile, "CONNECTION %D: %s\n", j + 1, connection);
        }

        j = 0;

        if (i == 0) {
            fprintf (roomFile, "ROOM TYPE: START_ROOM\n");
        } else if (i == (roomsNumber - 1)) {
            fprintf (roomFile, "ROOM TYPE: END_ROOM\n");
        } else {
            fprintf (roomFile, "ROOM TYPE: MID_ROOM\n");
        }

        fclose(roomFile);
    }
}

int main(void) {
    //Seed the random number generator.
    srand(getpid());

    //Create a directory for the rooms.
    int result = createDirectory();
    if (result != 0) {
        return 1;
    }

    //Define the names of ten rooms.
    const char *allRooms[] = {"copper", "notice", "truck", "branch", "doubt", "gold", "bell", "buzz", "swing", "bogus"};
    int RAND_ROOMS = 7;
    int MIN_CONNECTIONS = 3;

    //Generate indices of 7 random rooms.
    int randomRooms[RAND_ROOMS];
    getRandomRooms(randomRooms, RAND_ROOMS);

    //This 2d array will store each room's connections.
    //For example, connections[2] will represent all connections of the third room.
    int connections[RAND_ROOMS][RAND_ROOMS - 1];

    //Initialize connections array with empty tokens.
    initializeConnections(connections, RAND_ROOMS);

    //Generate random connections. Seven rooms will have three to six connections each.
    generateConnections(connections, RAND_ROOMS, MIN_CONNECTIONS);

    //Write each room connections into its own file.
    writeConnections(allRooms, randomRooms, connections, RAND_ROOMS);

    return 0;
}
