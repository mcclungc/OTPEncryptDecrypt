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

//functions returns 1(true) if first string is less than or equal size of second
int matchSize(char* plaintext, char* key)
{
      	int isMatch = 0;//bool
	if (strlen(plaintext) <= strlen(key))
	{
		isMatch = 1;//right size
	}
        return isMatch;//else wrong size
}

//function returns 1(true) if string contains illegal characters
int checkFileChars(char * filename){
	int i;
	int badChars = 0;//bool
	for(i = 0; i < strlen(filename);i++)
	{
		if((filename[i] < 'A' || filename[i] > 'Z') && (filename[i] != ' '))
		{
			//fprintf(stderr, "File contains bad characters!\n");
			badChars = 1;//bad characters are present
			return badChars;
		}
	}
	return badChars;//return default if conditional doesn't change it
}
int main(int argc, char *argv[])
{
	//variables for send/receive and sockets
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	
	//utility buffer and strings for plaintext and key
	char buffer[200001]; //using a really big buffer to be sure the whole file will fit
	char plainBuffer[200001]; //using a really big buffer to be sure the whole file will fit
	char keyBuffer[200001]; //using a really big buffer to be sure the whole file will fit
	
	//variables to pass plaintext and key sizes to server	
	int size, keysize;
		
	//argv[0] = otp_enc
	//argv[1] = plaintext file
	//argv[2] = key file
	//argv[3] = port 
	//hostname is "localhost"

	//check for correct number of arguments
	if (argc < 4) { fprintf(stderr,"USAGE: %s plaintext keyfile port\n", argv[0]); exit(0); } // Check usage & args

	//open and check plaintext and key file for bad characters, copying each to strings
	FILE *plainfd = fopen(argv[1], "r");//open plaintext for reading
	if (!plainfd)//check for successful file open
	{
		error("Could not open plaintext file.\n");	
	}
	
		
	memset(plainBuffer, '\0', sizeof(plainBuffer)); // Clear out the buffer array
	fgets(plainBuffer, sizeof(plainBuffer) - 1, plainfd); // Get input from the user, trunc to buffer - 1 chars, leaving \0
	plainBuffer[strcspn(plainBuffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds
	if (checkFileChars(plainBuffer)){//error and exit if plaintext has illegal characters
		fprintf(stderr, "File %s contains bad characters!\n",argv[1]);
		exit(1);
	}
	
	fclose(plainfd);//close file now contents are in string

	FILE *keyfd = fopen(argv[2], "r");//open key file for reading
	if (!keyfd)//check for successful file open
	{
		error("Could not open key file.\n");
	}
	
		
	memset(keyBuffer, '\0', sizeof(keyBuffer)); // Clear out the buffer array
	fgets(keyBuffer, sizeof(keyBuffer) - 1, keyfd); // Get input from the user, trunc to buffer - 1 chars, leaving \0
	keyBuffer[strcspn(keyBuffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds
	
	if (checkFileChars(keyBuffer)){//if file has bad characters, exit
		fprintf(stderr, "File %s contains bad characters!\n",argv[2]);
		exit(1);//exit if file contains illegal characters
	}
	
	fclose(keyfd); //close file now contents are in string

	if (!matchSize(plainBuffer, keyBuffer))//if key is too small, error and exit
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
	memset(buffer, '\0', sizeof(buffer)); // Clear out the utility buffer
	
	strcpy(buffer, "otp_enc");//move program name to buffer for send
	
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
		fprintf(stderr,"Error: This client %s was  not allowed to connect to server on port %s, exiting...\n",argv[0], argv[3]);fflush(stderr);
		exit(2);
	}

	// SEND PLAINTEXT
	
	//send plaintext size firsr
	
	size = strlen(plainBuffer);
	int n = send(socketFD,&size, sizeof(size),0);
	if (n< 0) error("CLIENT: ERROR writing to socket");
	
	//then send plaintext contents to server	
	charsWritten = send(socketFD, plainBuffer, strlen(plainBuffer), 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(plainBuffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

	//int checkSend = -5;  // Holds amount of bytes remaining in send buffer
	checkSend = -5;  // Holds amount of bytes remaining in send buffer
	do
	{
		ioctl(socketFD, TIOCOUTQ, &checkSend);  // Check the send buffer for this socket
	}
	while (checkSend > 0);  // Loop forever until send buffer for this socket is empty
	if (checkSend < 0) error("ioctl error");  // Check if we actually stopped the loop because of an error

	// Get return message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the utility buffer  for reuse
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");

	// SEND KEY
	
	//send keysize first
	keysize = strlen(keyBuffer);
	int m = send(socketFD,&keysize, sizeof(keysize),0);
	if (m < 0) error("CLIENT: ERROR writing to socket");

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

	// Get encoded message from server
	memset(buffer, '\0', sizeof(buffer)); // Clear out the utility buffer again for reuse
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");

	//printf("\nCLIENT: I received this from the server: \"%s\"\n", buffer);
	printf("%s\n", buffer);fflush(stdout);

	close(socketFD); // Close the socket
	return 0;//exit program successfully
}
