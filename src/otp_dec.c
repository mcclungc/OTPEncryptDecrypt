#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/ioctl.h>

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

//function returns 1(true) if 1st string is same size or smaller than 2nd
int matchSize(char* plaintext, char* key)
{
        int isMatch = 0;//set default to false
        if (strlen(plaintext) <= strlen(key))
        {
                isMatch = 1;//if right size, change to true
        }
        return isMatch;//return bop;
}
//function returns 1(true) if input string contains illegal charachters
int checkFileChars(char * filename){
        int i;
    	int badChars = 0;//set to false by default
	for(i = 0; i < strlen(filename);i++)
        {
                if((filename[i] < 'A' || filename[i] > 'Z') && (filename[i] != ' '))
		{
                        badChars = 1;//if illegal char, set to true and return badChars
			return badChars;
                }
	 }
	return badChars;//return default
}
int main(int argc, char *argv[])
{
	//variables for send receive and socket
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	
	//utility buffer and strings to hold encoded file and key
	char buffer[200001]; //using a really big buffer to be sure the whole file will fit
	char encodedBuffer[200001]; //using a really big buffer to be sure the whole file will fit
	char keyBuffer[200001]; //using a really big buffer to be sure the whole file will fit
		
	int size, keysize;// for tracking recv

	//argv[0] = otp_enc
	//argv[1] = plaintext file
	//argv[2] = key file
	//argv[3] = port 
	//hostname is "localhost"

	//check for correct number of variables
	if (argc < 4) { fprintf(stderr,"USAGE: %s ciphertext keyfile port\n", argv[0]); exit(0); } // Check usage & args

	//check encoded file and key for size and bad chars
	FILE *encodedfd = fopen(argv[1], "r");//open plaintext for reading
	if (!encodedfd)//check for successful file open
	{
		error("Could not open encoded text file.\n");
	}
	
		
	memset(encodedBuffer, '\0', sizeof(encodedBuffer)); // Clear out the buffer array
	fgets(encodedBuffer, sizeof(encodedBuffer) - 1, encodedfd); // Get input from the user, trunc to buffer - 1 chars, leaving \0
	encodedBuffer[strcspn(encodedBuffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds
	if (checkFileChars(encodedBuffer)){//check for bac chars
		fprintf(stderr, "File%s contains bad characters!\n", argv[1]);
		exit(1);//exit if contains bad chars
	}
	fclose(encodedfd);//close file now it is captured in a string


	FILE *keyfd = fopen(argv[2], "r");//open plaintext for reading
	if (!keyfd)//check for successul file open
	{
		error("Could not open key file.\n");
	}
		
	memset(keyBuffer, '\0', sizeof(keyBuffer)); // Clear out the buffer array
	fgets(keyBuffer, sizeof(keyBuffer) - 1, keyfd); // Get input from the user, trunc to buffer - 1 chars, leaving \0
	keyBuffer[strcspn(keyBuffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds
	if (checkFileChars(keyBuffer)){//check for bad chars
		fprintf(stderr, "File%s contains bad characters!\n", argv[2]);
		exit(1);//exit if contains bad chars
	}
	
	fclose(keyfd);//close file now contents are in string

	if (!matchSize(encodedBuffer, keyBuffer))//if key is too short, exit
        {
                fprintf(stderr, "Error - key is too short!\n"); fflush(stdout);
                exit(1);
        }

	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)serverHostInfo->h_addr, (char*)&serverAddress.sin_addr.s_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");

	// CHECK FOR VALID PROGRAM CONFIRMATION

	// Send program name to server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	
	strcpy(buffer, "otp_dec");//move name to buffer for send
	
	charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	int checkSend = -5;  // Holds amount of bytes remaining in send buffer
	do
	{
		ioctl(socketFD, TIOCOUTQ, &checkSend);  // Check the send buffer for this socket
	}
	while (checkSend > 0);  // Loop forever until send buffer for this socket is empty
	if (checkSend < 0) error("ioctl error");  // Check if we actually stopped the loop because of an error

	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
	if(strcmp(buffer, "I am the server, and you are forbidden to connect")==0)
	{
		
		fprintf(stderr,"This client %s is not permmitted to connect to server on port %s, exiting...\n", argv[0], argv[3]); fflush(stderr);
		exit(2);//exit if rejection message
	}

	// SEND ENCODED TEXT
	
	//send encoded string size
	size = strlen(encodedBuffer);
	int  n = send(socketFD, &size, sizeof(size),0);
	if (n < 0) error("CLIENT: ERROR writing to socket");

	charsWritten = send(socketFD, encodedBuffer, strlen(encodedBuffer), 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(encodedBuffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	checkSend = -5;  // Holds amount of bytes remaining in send buffer
	do
	{
		ioctl(socketFD, TIOCOUTQ, &checkSend);  // Check the send buffer for this socket
	}
	while (checkSend > 0);  // Loop forever until send buffer for this socket is empty
	if (checkSend < 0) error("ioctl error");  // Check if we actually stopped the loop because of an error

	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");

	// SEND KEY
	
	//send keysize first
	
	keysize = strlen(keyBuffer);
	int m = send(socketFD, &keysize, sizeof(keysize), 0);
	if (m < 0) error ("CLIENT: ERROR writing to socket");

	// Send key to server
	charsWritten = send(socketFD, keyBuffer, strlen(keyBuffer), 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(keyBuffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	checkSend = -5;  // Holds amount of bytes remaining in send buffer
	do
	{
		ioctl(socketFD, TIOCOUTQ, &checkSend);  // Check the send buffer for this socket
	}
	while (checkSend > 0);  // Loop forever until send buffer for this socket is empty
	if (checkSend < 0) error("ioctl error");  // Check if we actually stopped the loop because of an error

	// Get decoded message message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");

	printf("%s\n", buffer);fflush(stdout); //print decoded message

	close(socketFD); // Close the socket
	return 0;
}
