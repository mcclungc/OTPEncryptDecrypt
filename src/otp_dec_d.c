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

//function returns index in alphabet array fo charInpu
int getCharIndex(char charInput)
{
        int charIndex;
	int i;
        for (i = 0; i < (sizeof(alphabet)/sizeof(alphabet[0])); i++)
        {
                if (alphabet[i] == charInput)//find position in array
                {
                        charIndex = i;//set charIndex to correct position
                }
        }
        return charIndex;//return index
}

//function returns  index of decoded char using input encoded char and key char
int getDecodedIndex(int encodedIndex, int keyIndex)
{
        int decodedNum = (encodedIndex - keyIndex);
        if (decodedNum < 0) {decodedNum+=27;}//add 27 adjust input if negative number
        decodedNum = decodedNum % 27;//mod result
        return decodedNum;//return index of decoded char
}

int main(int argc, char *argv[])
{
	//variables to track child processes
	pid_t currentPid;
	int status;
	
	//variables for send receive and socket	
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	struct sockaddr_in serverAddress, clientAddress;
	
	//readBuffer for data from client, and 2 strings to copy plaintext and key into
	char readBuffer[200001];//utility buffer
	char encodedString[200001];//holds encoded string
	char keyString[200001];//holds key string
	char programName[1001];//holds program na,e

	//variables for tracking buffers
	int size = 0;//size encded string
	int keysize = 0;// size of key
	int readbytes = 0;

	//check for correct nnumber of arguments
	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

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
		if (establishedConnectionFD < 0) error("ERROR on accept");
		
		
                //fork
                currentPid = fork();
                if (currentPid ==-1)//if fork error, exit
                {       fprintf(stderr, "Fork error\n"); fflush(stderr);
                        exit(1);
                }
                else if (currentPid == 0) //child
                {
			// Get the program name from the client and display it
			memset(readBuffer, '\0', sizeof(readBuffer));
			charsRead = recv(establishedConnectionFD, readBuffer, 200000, 0); // Read the client's message from the socket
			if (charsRead < 0) error("ERROR reading from socket");
		
			memset(programName, '\0', sizeof(programName));
			strcpy(programName, readBuffer);//move program name to string for string comparison
	
			if (strcmp(programName,"otp_dec") != 0)//if not valid client, exit
			{
				fprintf(stderr,"Only otp_dec is allowed to connect!\n");//display error message
				// Send a rejection message back to the client
				charsRead = send(establishedConnectionFD, "I am the server, and you are forbidden to connect", 49, 0); // Send rejection back
				if (charsRead < 0) error("ERROR writing to socket");
				exit(2);//exit with exit value 2
			}

			charsRead = send(establishedConnectionFD, "I am the server, and you are allowed to connect", 47, 0); // Send success back
			if (charsRead < 0) error("ERROR writing to socket");

			// Get the encoded message from client
			//get size of encoded message first
			int n = recv(establishedConnectionFD, &size, sizeof(size), 0);
			if (n < 0) error("ERROR writing to socket");

			
			memset(readBuffer, '\0', sizeof(readBuffer));//clean out buffer
			//charsRead = recv(establishedConnectionFD, readBuffer, 200000, 0); // Read the client's message from the socket
			 do{
                                charsRead = recv(establishedConnectionFD, readBuffer, 200000, 0); // Read the client's message from the socket
                                if (charsRead < 0) error("ERROR reading from socket");
                                readbytes += charsRead;
                        } while (readbytes < size);//loop to get all data

		
		
			//copy readbuffer to string
			memset(encodedString, '\0', sizeof(encodedString));
			strcpy(encodedString, readBuffer);
		
			// Send a Success message back to the client
			charsRead = send(establishedConnectionFD, "I am the server, and I got your encoded text", 44, 0); // Send success back
			if (charsRead < 0) error("ERROR writing to socket");
		
			//Get key from client
			//first get keysize
			readbytes =0;
                        int m = recv(establishedConnectionFD, &keysize, sizeof(keysize), 0);
                        if (m < 0) error("ERROR writing to socket");
			
			memset(readBuffer, '\0', sizeof(readBuffer));	
			//charsRead = recv(establishedConnectionFD, readBuffer, 200000, 0); // Read the client's message from the socket
			do{
				charsRead = recv(establishedConnectionFD, readBuffer, 200000,0);
				if(charsRead < 0) error("ERROR reading from socket");
				readbytes += charsRead;
			} while (readbytes < keysize);//loop to get all data

		
			//copy readbuffer to string
			memset(keyString, '\0', sizeof(keyString));
			strcpy(keyString, readBuffer);//move key to string
                
			//DECODE ENCODED STRING
                	int i;
                	char decodedString[strlen(encodedString)];//string to hold encoded string
			memset(decodedString, '\0', sizeof(decodedString));
                	for (i = 0; i < strlen(encodedString); i++)
                	{
                        	int encodedNum = -1;
                        	char encodeLetter = encodedString[i]; //encoded char
                        	encodedNum = getCharIndex(encodeLetter);//index of encoded char

                        	int keyNum = -1;
                        	char keyLetter = keyString[i];//key char
                        	keyNum = getCharIndex(keyLetter);//index of key char

                        	int decodedNum = getDecodedIndex(encodedNum, keyNum);//index of decoded char


                        	decodedString[i]=alphabet[decodedNum];//write decoded char to string
                	}
			decodedString[strlen(encodedString)] = '\0';//null terminator

			//clean out buffer
			memset(readBuffer, '\0', sizeof(readBuffer));	
		
			// Send a Success message back to the client
			strcpy(readBuffer,decodedString);//move decoded message to buffer
			charsRead = send(establishedConnectionFD, readBuffer, 200000, 0); // Send decoded string back

			if (charsRead < 0) error("ERROR writing to socket");

			exit(0);
		}//end of child process
 		else //if parent, wait for child process to return
                {
                	while(currentPid > 0)
			{
				currentPid = waitpid(-1, &status, WNOHANG);//keep checking for child processes to finish  
			}
                }
		close(establishedConnectionFD); // Close the existing socket which is connected to the client
	}
		close(listenSocketFD); // Close the listening socket
		return 0;
}	

