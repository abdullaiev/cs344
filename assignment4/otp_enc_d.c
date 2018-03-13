#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>

//This is encryption SERVER.

// Error function used for reporting issues
void error(const char *msg) {
    perror(msg);
    exit(1);
}

char *getGoodChars() {
    return "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
}

int getCharIndex(char ch) {
    char *goodChars = getGoodChars();
    const int CHAR_QTY = strlen(goodChars);
    for (int i = 0; i < CHAR_QTY; i++) {
        if (goodChars[i] == ch) {
            return i;
        }
    }

    return -1;
}

int replyToClient(char *msg, int establishedConnectionFD) {
    int charsSent = send(establishedConnectionFD, msg, strlen(msg), 0);

    if (charsSent < 0) {
        error("ERROR writing to socket");
    }

    // Bytes remaining in send buffer
    int checkSend = -5;
    do {
        ioctl(establishedConnectionFD, TIOCOUTQ, &checkSend);  // Check the send buffer for this socket
    } while (checkSend > 0);  // Loop forever until send buffer for this socket is empty

    // Check if we actually stopped the loop because of an error
    if (checkSend < 0) {
        error("ioctl error");
    }

    // Close the existing socket which is connected to the client
    close(establishedConnectionFD);

    // Finish the child process
    exit(0);
}

int main(int argc, char *argv[]) {
    const int MAX_SIZE = 500000;
    int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
    socklen_t sizeOfClientInfo;
    char buffer[MAX_SIZE];
    struct sockaddr_in serverAddress, clientAddress;

    // Check usage & args
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

    // Set up the address struct for this process (the server)
    memset((char *) &serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
    portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string

    serverAddress.sin_family = AF_INET; // Create a network-capable socket
    serverAddress.sin_port = htons(portNumber); // Store the port number
    serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

    // Set up the socket
    listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
    if (listenSocketFD < 0) {
        error("ERROR opening socket");
    }

    // Enable the socket to begin listening
    if (bind(listenSocketFD, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        error("ERROR on binding");
    } // Connect socket to port

    listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

    while (1) {
        // Accept a connection, blocking if one is not available until one connects
        sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *) &clientAddress,
                                         &sizeOfClientInfo); // Accept

        if (establishedConnectionFD < 0) {
            error("ERROR on accept");
        }

        // Handle the rest of the transaction is a separate process
        pid_t forkProcessID = fork();
        int childExitMethod;

        switch (forkProcessID) {
            case -1: {
                error("Could not create a child process to handle decryption!");
                break;
            }
            case 0: {
                // This is the child process
                // Get the message from the client and display it
                memset(buffer, '\0', MAX_SIZE);
                charsRead = recv(establishedConnectionFD, buffer, 255, 0); // Read the client's message from the socket
                if (charsRead < 0) {
                    error("ERROR reading from socket");
                }
                printf("SERVER: I received this from the client: \"%s\"\n", buffer);

                //Verify connections comes from otp_enc.
                //This server expects the request text to be in format @@<plain_text>@@<key>@@
                if (buffer[0] != '@' || buffer[1] != '@') {
                    error("Connection is not from a trusted source!");
                    close(establishedConnectionFD);
                    exit(1);
                }

                //Save plain text first.
                char plainText[MAX_SIZE];
                int index = 2;
                int plainTextCount = 0;

                //Use buffer size to make sure the while loop doesn't go over buffer contents.
                int bufferSize = strlen(buffer);
                while (buffer[index] != '@' && index != bufferSize) {
                    plainText[plainTextCount] = buffer[index];
                    index++;
                    plainTextCount++;
                }

                //Increment the index to skip @@ between plain text and key.
                index += 2;

                //Get the key
                char key[MAX_SIZE];
                int keyCount = 0;
                while (buffer[index] != '@' && index != bufferSize) {
                    plainText[keyCount] = buffer[index];
                    index++;
                    keyCount++;
                }

                //Encrypt the text.
                int plainTextPosition;
                int keyPosition;
                int encryptedPosition;
                char encryptedMessage[MAX_SIZE];
                for (int i = 0; i < plainTextCount; i++) {
                    plainTextPosition = getCharIndex(plainText[i]);
                    keyPosition = getCharIndex(key[i]);
                    encryptedPosition = (plainTextPosition + keyPosition) % 27;
                    encryptedMessage[i] = getGoodChars()[encryptedPosition];
                }

                //Send the encrypted text to the client
                replyToClient(encryptedMessage, establishedConnectionFD);
            }
            default: {
                //This is the parent process.
                //Wait for the child process to finish so it does not become a zombie.
                //Do not block execution though to allow for multiple incoming requests.
                waitpid(forkProcessID, &childExitMethod, WNOHANG);
            }
        }
    }

    // Close the listening socket
    close(listenSocketFD);
    return 0;
}
