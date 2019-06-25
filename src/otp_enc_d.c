#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

//global alphabet array for use in encoding functions
char alphabet[27] = {'A','B','C','D','E','F','G','H','I','J',
                'K','L','M','N','O','P','Q','R','S','T',
                'U','V','W','X','Y','Z',' '};
//function returns index in alphabet of char input
int getCharIndex(char charInput)
{
        int charIndex;
	int i;	
        for (i = 0; i < (sizeof(alphabet)/sizeof(alphabet[0])); i++)
        {
                if (alphabet[i] == charInput)
                {
                        charIndex = i;
                }
        }
        return charIndex;
}
//function returns post encoded index of char from passed in plaintext and key indices
int getEncodedIndex(int plaintextIndex, int keyIndex)
{
        return((plaintextIndex + keyIndex) % 27);
}


int main(int argc, char *argv[])
{
	// variables for child process tracking
	pid_t currentPid;
	int status;

	//variables for send receive transactions and socket
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	struct sockaddr_in serverAddress, clientAddress;
	
	//readBuffer for data from client, and 2 strings to copy plaintext and key into
	char readBuffer[200001];
	char plainString[200001];
	char keyString[200001];
	char programName[1001];

	//variables for tracking buffer contents
	int size = 0;
	int keysize = 0;
	int readbytes = 0;
	
	//check for correct number of arguments	
	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1);fflush(stderr); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
	error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	while (1) {
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		//not sure if this should go here	
		if (establishedConnectionFD < 0) error("ERROR on accept");
		
		//fork
		currentPid = fork();
		if (currentPid ==-1)//if unsucessful fork exit process
		{	
			fprintf(stderr, "Fork error\n"); fflush(stderr);
			exit(1);
		}		
		else if (currentPid == 0) //child
		{
		
			// Get the program name from the client and display it
			memset(readBuffer, '\0', sizeof(readBuffer));
			charsRead = recv(establishedConnectionFD, readBuffer, 200000, 0); // Read the client's message from the socket
			if (charsRead < 0) error("ERROR reading from socket");
		
			memset(programName, '\0', sizeof(programName));//clean out buffer
			strcpy(programName, readBuffer);//copy program name to string for string comparison
			
			if (strcmp(programName,"otp_enc") != 0)//if program name not a match
			{
				//fprintf(stderr,"Only otp_enc is allowed to connect!\n");fflush(stderr);//message that connection is not allowed
				// Send a rejection message back to the client
				charsRead = send(establishedConnectionFD, "I am the server, and you are forbidden to connect", 49, 0); // Send rejection back
				if (charsRead < 0) error("ERROR writing to socket");
				exit(2);//exit if invalid client
			}

			charsRead = send(establishedConnectionFD, "I am the server, and you are allowed to connect", 47, 0); // Send success back
			if (charsRead < 0) error("ERROR writing to socket");

			// Get the plaintext message from the client
			//first get size
			int n = recv(establishedConnectionFD, &size, sizeof(size), 0);
			if (n < 0) error("ERROR writing to socket");
			
			memset(readBuffer, '\0', sizeof(readBuffer));//clean out buffer
			do{
				charsRead = recv(establishedConnectionFD, readBuffer, 200000, 0); // Read the client's message from the socket
				if (charsRead < 0) error("ERROR reading from socket");//if read error
				readbytes += charsRead;
			} while (readbytes < size);//loop to get everythign 		
			
			//printf("Chars Received: %d\n",charsRead);fflush(stdout);
		
			//copy readbuffer to string
			memset(plainString, '\0', sizeof(plainString));//initialize
			strcpy(plainString, readBuffer);
		
			// Send a Success message back to the client
			charsRead = send(establishedConnectionFD, "I am the server, and I got your plaintext", 41, 0); // Send success back
			if (charsRead < 0) error("ERROR writing to socket");
		
			//Get the key from client
			//first get keysize
			readbytes =0;//reset counter to track bytes read from new file
			int m = recv(establishedConnectionFD, &keysize, sizeof(keysize), 0);
			if (m < 0) error("ERROR writing to socket");
			
			memset(readBuffer, '\0', sizeof(readBuffer));//clean out buffer
			do{
				charsRead = recv(establishedConnectionFD, readBuffer, 200000, 0); // Read the client's message from the socket
				if (charsRead < 0) error("ERROR reading from socket");
				readbytes += charsRead;
			} while (readbytes < keysize);//loop to get all data
			
			//printf("Chars Received: %d\n",charsRead);fflush(stdout);
		
			//copy readbuffer to string
			memset(keyString, '\0', sizeof(keyString));//initialize
			strcpy(keyString, readBuffer);
		
			//ENCODE PLAINTEXT
			int i;
        		int plaintextNum, keyNum; //variables for index in alphabet array
			char encodedString[strlen(plainString)];//holds encoded string
			memset(encodedString, '\0', sizeof(encodedString));//initialize it
        		for (i = 0; i < strlen(plainString); i++)
        		{
                		plaintextNum = -1;
                		char letter = plainString[i];//char from plaintext
                		plaintextNum = getCharIndex(letter);//index of char in alphabet

                		keyNum = -1;
                		char keyLetter = keyString[i];//char from key
                		keyNum = getCharIndex(keyLetter);//index of char in alphabet

                		int encodeNum = getEncodedIndex(plaintextNum, keyNum);//index of encoded char in alphabet array

                		encodedString[i]=alphabet[encodeNum];

       		 	}	
			encodedString[strlen(plainString)] = '\0';//null terminate the string

			//clean out buffer
			memset(readBuffer, '\0', sizeof(readBuffer));	
			strcpy(readBuffer,encodedString);//move encoded string to buffer	

			// Send a Success message back to the client
			charsRead = send(establishedConnectionFD, readBuffer , 200000, 0); // Send encoded string back

			if (charsRead < 0) error("ERROR writing to socket");

			exit(0);
		}//end of child process
		
		else //if parent, wait for child process to return 
		{
			while(currentPid > 0)
			{
				currentPid = waitpid(-1, &status, WNOHANG);//keep checking for background child processes
			}
		}			
		close(establishedConnectionFD); // Close the existing socket which is connected to the client
		
	}
		close(listenSocketFD); // Close the listening socket
		return 0;
}	
