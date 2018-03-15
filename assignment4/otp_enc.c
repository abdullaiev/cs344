#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>

#define h_addr h_addr_list[0] /* for backward compatibility */

//This is encryption CLIENT.
//This program reads plain text form the first param file.
//It then sends that plaintext along with the key to encryption server to do the actual encryption.
//Once ciphertext is received back from server, it is printed out to stdout.

// Error function used for reporting issues
void error(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

//A helper function that checks if given string contains any non-accepted chars such as lower case letters or special symbols.
void checkForBadChars(char *str) {
    char *goodChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
    size_t strLength = strlen(str);
    size_t CHAR_QTY = strlen(goodChars);
    for (int i = 0; i < strLength; i++) {
        int good = 0;

        for (int j = 0; j < CHAR_QTY; j++) {
            if (str[i] == goodChars[j]) {
                good = 1;
                break;
            }
        }

        if (good == 0) {
            error("otp_enc error: input contains bad characters");
        }
    }
}

int main(int argc, char *argv[]) {
    int socketFD, portNumber;
    struct sockaddr_in serverAddress;
    struct hostent *serverHostInfo;
    int MAX_SIZE = 150000;
    char buffer[MAX_SIZE];
    char plainText[MAX_SIZE];
    char key[MAX_SIZE];

    if (argc < 4) {
        fprintf(stderr, "USAGE: %s <plaintext> <key> <port>\n", argv[0]);
        exit(1);
    }

    //Open plaintext file and save its contents into plainText variable.
    FILE *filePointer = fopen(argv[1], "r");
    if (filePointer == NULL) {
        error("Could not open plain text file to read.");
    }
    fgets(plainText, MAX_SIZE, filePointer);
    fclose(filePointer);

    //Remove trailing new lines chars
    size_t length = strlen(plainText);
    if (plainText[length - 1] == '\n') {
        plainText[length - 1] = '\0';
    }

    //Check that plain text does not contain bad characters
    checkForBadChars(plainText);

    //Read the key from provided file.
    filePointer = fopen(argv[2], "r");
    if (filePointer == NULL) {
        error("Could not open key file to read.\n");
    }
    fgets(key, MAX_SIZE, filePointer);
    fclose(filePointer);

    //Remove trailing new lines chars
    length = strlen(key);
    if (key[length - 1] == '\n') {
        key[length - 1] = '\0';
    }

    //Make sure that the key does not have bad characters either
    checkForBadChars(key);

    //Check that the key length is at least the same as plain text's length.
    char errorMsg[MAX_SIZE];
    if (strlen(key) < strlen(plainText)) {
        sprintf(errorMsg, "Error: key ‘%s’ is too short\n", argv[2]);
        error(errorMsg);
    }

    // Set up the server address struct
    memset((char *) &serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
    portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
    serverAddress.sin_family = AF_INET; // Create a network-capable socket
    serverAddress.sin_port = htons(portNumber); // Store the port number
    serverHostInfo = gethostbyname("127.0.0.1"); // Convert the machine name into a special form of address

    if (serverHostInfo == NULL) {
        fprintf(stderr, "CLIENT: ERROR, no such host\n");
        exit(1);
    }

    memcpy((char *) &serverAddress.sin_addr.s_addr, (char *) serverHostInfo->h_addr,
           serverHostInfo->h_length); // Copy in the address

    // Set up the socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0) {
        error("CLIENT: ERROR opening socket");
    }

    // Connect to server
    if (connect(socketFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        error("CLIENT: ERROR connecting");
    }

    //Clear out the buffer array
    memset(buffer, '\0', sizeof(buffer));

    //Save the plain text and the key to be sent to the server in "@@<plain_text>&&<key>@@" format/
    sprintf(buffer, "@@%s&&%s@@", plainText, key);

    // Send message to server
    ssize_t charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
    if (charsWritten < 0) {
        error("CLIENT: ERROR writing to socket");
    }

    //Make sure it's sent completely.
    int checkSend = -5;
    do {
        ioctl(socketFD, TIOCOUTQ, &checkSend);  // Check the send buffer for this socket
    } while (checkSend > 0);  // Loop forever until send buffer for this socket is empty

    // Check if we actually stopped the loop because of an error
    if (checkSend < 0) {
        error("ioctl error");
    }

    // Get return message from server
    memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
    ssize_t charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end

    if (charsRead < 0) {
        error("CLIENT: ERROR reading from socket");
    }

    //Auth error occurred. Print out a message and exit with code 2.
    if (buffer[0] == '!') {
        fprintf(stderr, "Error: could not contact otp_enc_d on port %s\n", argv[3]);
        exit(2);
    }

    //Print out the received text to stdout. It should print the ciphertext.
    printf("%s\n", buffer);

    //Close the connection.
    close(socketFD);
    return 0;
}
