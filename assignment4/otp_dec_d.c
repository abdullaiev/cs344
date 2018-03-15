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

//This is decryption SERVER.
//It receives ciphertext and key from client and performs actual decryption.
//Plain text is being sent to the client then.

// Error function used for reporting issues
void error(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

//A set of chars accepted by the program.
char *getGoodChars() {
    return "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
}

//A helper function to return a char's index in the array of accepted chars above.
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

//A helper function that sends a message back to client within the given communication socket.
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
    const int MAX_SIZE = 150000;
    const int PROCESSES = 2048;
    pid_t backgroundProcessIDs[PROCESSES];
    int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
    socklen_t sizeOfClientInfo;
    char buffer[MAX_SIZE];
    char completeMessage[MAX_SIZE];
    struct sockaddr_in serverAddress, clientAddress;

    // Check usage & args
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

    //Initialize process arrays with zero values. It will show that the element by certain index has not been initialized.
    for (int i = 0; i < PROCESSES; i++) {
        backgroundProcessIDs[i] = 0;
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
        //Check for any completed background processes.
        for (int i = 0; i < PROCESSES; i++) {
            if (backgroundProcessIDs[i] != 0) {
                //Do not block execution of this parent process if child has not completed yet.
                int existStatus;
                int processCompleted = waitpid(backgroundProcessIDs[i], &existStatus, WNOHANG);
                if (processCompleted != 0) {
                    if (WIFEXITED(existStatus)) {
                        backgroundProcessIDs[i] = 0;
                    } else if (WIFSIGNALED(existStatus)) {
                        backgroundProcessIDs[i] = 0;
                    }
                }
            }
        }

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
                memset(completeMessage, '\0', MAX_SIZE);
                //Read the client's message from the socket until the message is read in full.
                while(completeMessage[strlen(completeMessage) - 1] != '@') {
                    memset(buffer, '\0', MAX_SIZE);
                    charsRead = recv(establishedConnectionFD, buffer, MAX_SIZE - 1, 0);
                    if (charsRead < 0) {
                        error("ERROR reading from socket");
                    }
                    sprintf(completeMessage, "%s%s", completeMessage, buffer);
                }

                //Copy complete message back to buffer
                memset(buffer, '\0', MAX_SIZE);
                sprintf(buffer, "%s", completeMessage);

                //Verify connections comes from otp_enc.
                //This server expects the request text to be in format ^^<cipher_text>&&<key>@@.
                //Reject this request if the format is different.
                if (buffer[0] != '^' || buffer[1] != '^') {
                    replyToClient("!", establishedConnectionFD);
                }

                //Save ciphertext first.
                char cipherText[MAX_SIZE];
                int index = 2;
                int cipherTextCount = 0;

                //Use buffer size to make sure that while loop doesn't go over buffer contents.
                int bufferSize = strlen(buffer);
                while (buffer[index] != '&' && index != bufferSize) {
                    cipherText[cipherTextCount] = buffer[index];
                    index++;
                    cipherTextCount++;
                }
                cipherText[cipherTextCount] = '\0';

                //Increment the index to skip && between ciphertext and key.
                index += 2;

                //Get the key
                char key[MAX_SIZE];
                int keyCount = 0;
                while (buffer[index] != '@' && index != bufferSize) {
                    key[keyCount] = buffer[index];
                    index++;
                    keyCount++;
                }
                key[keyCount] = '\0';

                //Decrypt the text.
                int cipherTextPosition;
                int keyPosition;
                int plainTextPosition;
                char plainText[MAX_SIZE];
                int i = 0;
                for (; i < cipherTextCount; i++) {
                    cipherTextPosition = getCharIndex(cipherText[i]);
                    keyPosition = getCharIndex(key[i]);
                    plainTextPosition = (cipherTextPosition - keyPosition) % 27;
                    //Make sure the index is positive.
                    if (plainTextPosition < 0) {
                        plainTextPosition += 27;
                    }
                    plainText[i] = getGoodChars()[plainTextPosition];
                }
                plainText[i] = '\0';

                //Send the encrypted text to the client
                replyToClient(plainText, establishedConnectionFD);
            }
            default: {
                //This is the parent process.
                //Save child process ID so it can be killed later once it's done.
                //Do not block execution though to allow for multiple incoming requests.
                int countP = 0;
                while (backgroundProcessIDs[countP] != 0) {
                    countP++;
                }
                backgroundProcessIDs[countP] = forkProcessID;
            }
        }
    }

    // Close the listening socket
    close(listenSocketFD);
    return 0;
}
