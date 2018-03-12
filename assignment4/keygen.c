#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//Generate a random key for one-time pod.
//The length of the key is the first parameter passed to the program.
//The program will output the key to the stdout unless it's redirected.

//Returns a random number in range min...max.
int getRandomNumberInRange(int min, int max) {
    return rand() % (max + 1 - min) + min;
}

int main(int argc, const char* argv[]) {
    //Seed the random number generator.
    srand(time(NULL));

    //All possible chars for the key. 26 English alphabet letters and a space.
    char *chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    if (argc < 2) {
        fprintf(stderr, "SYNTAX: %s <keylength>\n", argv[0]);
        exit(1);
    }

    //Convert key length from string to integer.
    int length = atoi(argv[1]);

    //Generate a random key.
    char key[length];
    int count = 0;

    for (; count < length; count++) {
        int index = getRandomNumberInRange(0, 26);
        key[count] = chars[index];
    }

    //Null terminate the key string.
    key[count] = '\0';

    //Output the key to stdout, add a new line character at the end.
    printf("%s\n", key);
    return 0;
}