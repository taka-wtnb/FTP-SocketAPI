/******************************************************************************************************************************
** Author: Takahiro Watanabe
** Program name: CS372 Project 2 ftserver (with extra credit feature)
** Course Name: CS372 Introduction to Computer Networks (Fall 2019)
** Last Modified: 12/01/19
** Description: This program runs as a server host of a file transfer application. This one works paired with a client 
**              host, which is ftclient. ftserver accepts a connection from ftclient to establish the TCP control connection
**              and receives a request. Then it connects to ftclient to open the TCP data connection,
**              then sends the requested file data to ftclient, if appropriate.
**		This file also implements one extra credit feature. For details of the extra credit feature, 
**		please refer to README.txt included in the submission zip file.
**              This program is written in C and implements the socket API and TCP protocol.
** Source 1: Sample code used in CS344 (Fall 2018) Block 4 Lecture Slide ("Lecture 4.3 Network Servers")
** Source 2: My own code written for CS372 (Fall 2019)'s Project 1 assignment
** Source 3: <Title> Catch Ctrl-C in C
**           <Author> icyrock.com and Vukašin Manojlović
**           <Date of Retrieval> 11/23/19
**           <Availability> https://stackoverflow.com/questions/4217037/catch-ctrl-c-in-c/4217052
** Source 4: <Title> Returning string from C function
**           <Author> michaelmeyer
**           <Date of Retrieval> 11/27/19
**           <Availability> https://stackoverflow.com/questions/25798977/returning-string-from-c-function
** Source 5: <Title> How to play with strings in C
**           <Author> Donotalo
**           <Date of Retrieval> 11/27/19
**           <Availability> https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/string-split
** Source 6: <Title> C Program to list all files and sub-directories in a directory
**           <Author> GeeksforGeeks
**           <Date of Retrieval> 11/27/19
**           <Availability> https://www.geeksforgeeks.org/c-program-list-files-sub-directories-directory/
** Source 7: <Title> Send large files over socket in C
**           <Author> Fred
**           <Date of Retrieval> 11/28/19
**           <Availability> https://stackoverflow.com/questions/8679547/send-large-files-over-socket-in-c
** Source 8: <Title> Proper way to get file size in C
**           <Author> StackPeter
**           <Date of Retrieval> 11/28/19
**           <Availability> https://stackoverflow.com/questions/35390912/proper-way-to-get-file-size-in-c
** Source 9: <Title> Strings with malloc in C
**           <Author> Lacobus
**           <Date of Retrieval> 11/28/19
**           <Availability> https://stackoverflow.com/questions/36694345/strings-with-malloc-in-c
** Source 10: <Title> (C Programming) User name and Password Identification
**            <Author> Maky
**            <Date of Retrieval> 11/30/19
**            <Availability> https://stackoverflow.com/questions/15790621/c-programming-user-name-and-password-identification
******************************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <dirent.h>


// Function to catch a SIGINT signal (CTRL-C) and exit the program
// This takes a signal as parameter and returns nothing.
// Pre-conditions: the program is still running
// Post-conditions: print a message on console and exit the program
void INThandler(int sig)
{
	// upon receiving SIGINT, it prints the terminating message and exits the program.
	signal(sig, SIG_IGN);
	printf("\nftserver terminated by SIGINT.\n\n");
	exit(0);
}


// Error function used for reporting issues and exiting the program. 
// This is a void function and takes a C string as a parameter.
// Pre-conditions: the program is still running
// Post-conditions: print a message on console and exit the program
void error(const char *msg) 
{ 
 	perror(msg); 
 	exit(0); 
} 


// Function to validate the user-input server port number. If the number is invalid, 
// it displays an error message and prompts the user for another input. If valid, it returns the validated port number.
// This function returns an int and takes no parameter.
// Pre-conditions: the user provided the un-validated server port number
// Post-conditions: the port number is either validated or invalidated
int validParam(int tempPortNum)
{
	// flag for the valid port number
	int portFlag = 0;

	// the server port number must be in the valid range (0 - 65535). if not, display an error message and chnage the flag.
	if (tempPortNum < 0 || tempPortNum > 65535)
	{
		printf("The port number is out of range. Cannot be used.\n");
		portFlag = 0;
	}
	// it also cannot use the reserved port number (0 - 1023).
	else if (0 <= tempPortNum && tempPortNum <= 1023)
	{
		printf("The port number is reserved. Cannot be used.\n");
		portFlag = 0;
	}
	// it should avoid using 30020 and 30021 too, because they are well-known FTP port number.
	else if (tempPortNum == 30021 || tempPortNum == 30020)
	{
		printf("You must avoid using the port number (well-known FTP port number).\n");
		portFlag = 0;
	}
	// if valid, set the flag to 1
	else
	{
		portFlag = 1;
	}

	// if invalid, prompt the user for another input until it gets the valid port number
	while (portFlag == 0)
	{
		printf("Enter the port number again: ");
		scanf("%d", &tempPortNum);

		// the server port number must be in the valid range (0 - 65535). if not, display an error message and chnage the flag.
		if (tempPortNum < 0 || tempPortNum > 65535)
		{
			printf("The port number is out of range Cannot be used.\n");
			portFlag = 0;
		}
		// it also cannot use the reserved port number (0 - 1023).
		else if (0 <= tempPortNum && tempPortNum <= 1023)
		{
			printf("The port number is reserved. Cannot be used.\n");
			portFlag = 0;
		}
		// it should avoid using 30020 and 30021 too, because they are well-known FTP port number.
		else if (tempPortNum == 30021 || tempPortNum == 30020)
		{
			printf("You must avoid using the port number (well-known FTP port number).\n");
			portFlag = 0;
		}
		// if valid, set the flag to 1
		else
		{
			portFlag = 1;
		}
	}

	// return the validated port number
	return tempPortNum;
}

// ***** This is the implemantation for extra credit *****
// Function to prompt the user for the username and password to access ftserver and validate it.
// This is a void function and takes no parameter.
// Pre-conditions: the program has started, but doesn't do anything meaningful until the user provides valid credentials
// Post-conditions: once the credentials validated, the program goes ahead and sets up the socket for control connection
void usernamePasswordEC()
{
	char userName[100];
	char passWord[100];
	char validUN[] = "OSUBeavers";
	char validPW[] = "CS372@Fall2019";

	// Get a username from user
	printf("Please enter your username, then hit enter: ");

	memset(userName, '\0', sizeof(userName));		// Clear out the userName array
	fgets(userName, sizeof(userName) - 1, stdin);		// Get input from the user, trunc to buffer - 1 chars, leaving \0
	userName[strcspn(userName, "\n")] = '\0';		// Remove the trailing \n that fgets adds

	// Then, get a passwer from user
	printf("Please enter your password, then hit enter: ");

	memset(passWord, '\0', sizeof(passWord));		// Clear out the passWord array
	fgets(passWord, sizeof(passWord) - 1, stdin);		// Get input from the user, trunc to buffer - 1 chars, leaving \0
	passWord[strcspn(passWord, "\n")] = '\0';		// Remove the trailing \n that fgets adds

	// validate the credentials until the user provides the valid username & password pair
	while (!((strcmp(userName, validUN) == 0) && (strcmp(passWord, validPW) == 0)))
	{
		printf("\nThe username & password pair is invalid!\n\n");
		
		// Get a username from user again
		printf("Please enter your username again, then hit enter: ");
		memset(userName, '\0', sizeof(userName));	// Clear out the userName array
		fgets(userName, sizeof(userName) - 1, stdin);	// Get input from the user, trunc to buffer - 1 chars, leaving \0
		userName[strcspn(userName, "\n")] = '\0';	// Remove the trailing \n that fgets adds

		// Then, get a passwer from user again
		printf("Please enter your password again, then hit enter: ");
		memset(passWord, '\0', sizeof(passWord));	// Clear out the passWord array
		fgets(passWord, sizeof(passWord) - 1, stdin);	// Get input from the user, trunc to buffer - 1 chars, leaving \0
		passWord[strcspn(passWord, "\n")] = '\0';	// Remove the trailing \n that fgets adds
	}
	// print the success message if the credential is valid
	printf("\nSuccess! Server access granted.\n");
}


// Function to start accepting the control connection from ftclient, creating and binding a socket for ftclient.
// Then start listening on the specified server port.
// This function takes a reference to int (socket) and an int parameter and returns nothing.
// Pre-conditions: the socket for control connection is not yet open on the ftserver's side
// Post-conditions: ftserver's socket for control connection is open and starts listening on the specified port
void startUp(int* listenSocketFD, int portNumber)
{
	struct sockaddr_in serverAddress;
	   	 
	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	
	serverAddress.sin_family = AF_INET;		// Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); 	// Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; 	// Any address is allowed for connection to this process

	// Set up the socket
	*listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	// error checking for opening the socket
	if (*listenSocketFD < 0)
	{
		fprintf(stderr, "ERROR opening socket\n");
		exit(1);
	}

	// Enable the socket to begin listening
	if (bind(*listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
	{
		fprintf(stderr, "ERROR on binding\n");
		exit(1);
	}
}


// Function to get messages from ftclient.
// This is a void function and takes the socket FD, a C-string, and an int as a parameter.
// Pre-conditions: ftserver has not read a message from its receiving buffer 
// Post-conditions: if successful, the message is read and displayed on console.
void receiveMessage(int socketFD, char *message, int length)
{
	// set up the receiving buffer, some C strings, etc.
	int charsRead;

	memset(message, '\0', length);				// Clear out the buffer
	charsRead = recv(socketFD, message, length - 1, 0);	// Read data from the socket, leaving \0 at end

	// Error checking for reading from socket
	if (charsRead < 0)
	{
		error("SERVER: ERROR reading from socket");
	}
}


// Function to initiate a contact from ftserver to ftclient to establish the data connection.
// This returns an int and takes the socketFD (int) and the address struct as the parameters.
// Pre-conditions: a socket for the data connection with ftclient is open, but not yet submitted
// Post-conditions: data connection request is submitted to ftclient
int initContact(int socketFD, struct sockaddr_in clientAddress)
{
	int connectData = 1;	// flag for successful connection
	
	// Connect socket to address and check for the error
	// if connection fails, flip the flag to 0
	if (connect(socketFD, (struct sockaddr*)&clientAddress, sizeof(clientAddress)) < 0)
	{
		printf("Server: ERROR connecting to ftclient for the data connection\n");
		connectData = 0;
	}
	// return the flag value
	return connectData;
}


// Function to get names of files and directories in ftserver's current directory.
// This returns an int and takes an array of C-string as a parameter.
// pre-condition: the list of file (and directory) names is unknown. the number of files (and directories) is also unknown.
// post-condition: the list of file (and directory) names is known. the number of files (and directories) is also known.
int listFiles(char* fileList[])
{
	struct dirent *de;  	// Pointer for directory entry 
	int fileCount = 0;	// holds the number of files (and directories)

	// look at the current directory. opendir() returns a pointer of DIR type.  
	DIR *dr = opendir(".");

	// opendir returns NULL if couldn't open directory 
	if (dr == NULL)  
	{
		printf("Could not open current directory.\n");
	}

	// read the file (or directory) names inside the current directory one by one
	// then insert the name into the array. also update the file count.
	while ((de = readdir(dr)) != NULL)
	{
		fileList[fileCount] = malloc(1024 * sizeof(char));	
		strcpy(fileList[fileCount], de->d_name);
		fileCount++;
	}
	// close the current directory
	closedir(dr);

	// return the file count
	return fileCount;
}


// Function to send a message from ftserver to ftclient
// This is a void function and takes the socket FD and C-string as parameters.
// Pre-conditions: connection is established between ftserver and ftclient and sending buffer is provided with some message.
// Post-conditions: if successful, the message in the buffer is sent to ftclient.
void sendMessage(int socketFD, char* message)
{
	// variable to tell successful sending.
	int charsWritten;

	// Send message to ftclient
	charsWritten = send(socketFD, message, strlen(message), 0);	// Write to ftclient

	// Error checking for writing to socket
	if (charsWritten < 0)
	{
		error("Server: ERROR writing to socket");
	}

	// Error checking for the message length which is actually sent
	if (charsWritten < strlen(message))
	{
		printf("Server: WARNING: Not all data written to socket!\n");
	}
}


// Function to process the request sent from ftclient over control connection and process it.
// if appropriate, it sends the requested file data to ftclient over data connection.
// This is a void function and takes an int (socket FD), three C-strings, array of C-strings, two other ints.
// Pre-conditions: ftclient sends the proper request command over control connection 
// and the data connection is successfully established.
// Post-conditions: if appropriate, ftserver sends the requested file data to ftclient
void handleRequest(int socketFD, char* command, char** fileList, int fileCount, char* fileName, char* hostName, int dataPortNum)
{
	// declare some variables for file descriptor, sending & receiving buffers, etc.
	int charsRead, fileFD;
	char sendBuff[1024];
	char rcvBuff[10];

	memset(sendBuff, '\0', sizeof(sendBuff));	// Clear out the buffer
	memset(rcvBuff, '\0', sizeof(rcvBuff));		// Clear out the buffer

	// handle the case where the requested command is '-l'
	if (strcmp(command, "-l") == 0)
	{
		printf("List directory requested on port %d\n", dataPortNum);

		// holds the number of the files (and directories) in ftserver's directory
		char countBuff[12];
		memset(countBuff, '\0', sizeof(countBuff));	// Clear out the buffer

		// convert int to string
		sprintf(countBuff, "%d", fileCount);

		printf("Sending directory contents to %s:%d\n", hostName, dataPortNum);

		// send the file count info to ftclient then receive the aknowledgement from it
		sendMessage(socketFD, countBuff);
		receiveMessage(socketFD, rcvBuff, sizeof(rcvBuff));
		
		// send the file (and directory) name in ftserver's directory to ftclient one by one
		int i;
		for (i = 0; i < fileCount; i++)
		{
			memset(sendBuff, '\0', sizeof(sendBuff));	// Clear out the buffer
			
			// send the file (and directory) name to ftclient then receive the acknowledgement from it each by each
			strcpy(sendBuff, fileList[i]);
			sendMessage(socketFD, sendBuff);
			receiveMessage(socketFD, rcvBuff, sizeof(rcvBuff));
		}
	}
	// handle the case where the requested command is '-g'
	else if (strcmp(command, "-g") == 0)
	{
		memset(sendBuff, '\0', sizeof(sendBuff));		// Clear out the buffer
		
		// first, send the file name to ftclient and receive the acknowledgement from it 
		strcpy(sendBuff, fileName);
		sendMessage(socketFD, sendBuff);
		receiveMessage(socketFD, rcvBuff, sizeof(rcvBuff));

		// if the acknowledgement is a positive one (i.e. no duplicate file), send the file date to ftclient
		if (strcmp(rcvBuff, "OK") == 0)
		{
			printf("Sending \"%s\" to %s:%d.\n", fileName, hostName, dataPortNum);
		
			// open the file to get the byte count info
			FILE* fp;
			fp = fopen(fileName, "r");

			// use fseek to count the byte in the file and get the point back to the beginning of the file,
			// the close the file
			fseek(fp, 0, SEEK_END);
			long int fileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			fclose(fp);
		
			char byteBuff[100];				// holds the byte count info
			memset(byteBuff, '\0', sizeof(byteBuff));	// Clear out the buffer

			// convert long int to string
			sprintf(byteBuff, "%ld", fileSize);

			// send the byte count info to ftclient then receive the acknowledgement from it
			sendMessage(socketFD, byteBuff);
			receiveMessage(socketFD, rcvBuff, sizeof(rcvBuff));

			memset(sendBuff, '\0', sizeof(sendBuff));	// Clear out the buffer

			// open the file for reading
			fileFD = open(fileName, O_RDONLY);
	
			// Keep reading the file until all the bytes are read
			while (charsRead = read(fileFD, sendBuff, sizeof(sendBuff)-1) > 0)
			{
				// send the file data to ftclient chunk by chunk and receive the aknowledgement from it
				sendMessage(socketFD, sendBuff);
				receiveMessage(socketFD, rcvBuff, sizeof(rcvBuff));
			}
			// close the file
			close(fileFD);
		}
		// if receiving a negative aknowledgement from ftclient, it means there is the duplicated file.
		// print the request cancel message.
		else if (strcmp(rcvBuff, "NO") == 0)
		{
			printf("File request cancelled by ftclient due to duplicate files.\n");
		}
	}
}


int main(int argc, char *argv[])
{
	signal(SIGINT, INThandler);		// function to handle the SIGINT (Ctrl+C)

	//declare some variable used for the socket API, sending/receiving buffers, and C strings, etc.
	int listenSocketFD, establishedConnectionFD, socketFD, tempPortNum, portNumber, charsRead, charsWritten;
	socklen_t sizeOfClientInfo;
	struct sockaddr_in clientAddress;
	struct hostent* clientHostInfo;
	char hostName[6];
	char host[1024];
	char service[20];
	char paramBuff[1024];
	char ftCommand[10];
	char list[] = "-l";
	char trans[] = "-g";
	char dataPortBuff[10];
	int dataPortNum;
	char fileName[1024];
	char* fileList[999] = {'\0'};
	int fileCount = 0;
	int i;
	int fileExist = 1;
	char noFileMsg[] = "FILE NOT FOUND";
	char yesFileMsg[] = "OK";


	// Check usage & args
	if (argc < 2)
	{
		fprintf(stderr, "USAGE: %s port [&]\n", argv[0]);
		exit(1);
	}

	// Get the port number, convert to an integer from a string
	tempPortNum = atoi(argv[1]); 

	// call a funtion to valiate the port number
	portNumber = validParam(tempPortNum);

	// ***** This is the implemantation for extra credit *****
	// call a function to ask the user for credentials to access ftserver and validate it
	usernamePasswordEC();

	// call a function to create a socket for control connection
	startUp(&listenSocketFD, portNumber);
	printf("\nServer open on %d\n", portNumber);
	
	// Flip the socket on and start to listen to the connection request
	listen(listenSocketFD, 1); 

	// loop to accept the control connection from ftclient continuously
	while (1)
	{	
		// flag to tell if ftserver has the requested file
		fileExist = 1;
		
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress);		// Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);		// Accept

		// error checking for establishing the control connection
		if (establishedConnectionFD < 0)
		{
			fprintf(stderr, "ERROR on accept\n");
			exit(1);
		}

		// get the host name info of ftclient
		getnameinfo((struct sockaddr *)&clientAddress, sizeof clientAddress, host, sizeof host, service, sizeof service, 0);
		strcpy(hostName, host);
		hostName[5] = '\0';
		
		// hostName now holds the client host name and display it on screen
		printf("\nConnection from %s\n", hostName);

		memset(paramBuff, '\0', sizeof(paramBuff));			// Clear out the buffer
		// receive the requested file command and its arguments from frclient
		receiveMessage(establishedConnectionFD, paramBuff, sizeof(paramBuff));

		// parse the command sent from ftclient
		char delim[] = " ";
		char* ptr = strtok(paramBuff, delim);
	
		memset(ftCommand, '\0', sizeof(ftCommand));			// Clear out the buffer
		strcpy(ftCommand, ptr);

		// handle the case where the command is '-l'
		if (strcmp(ftCommand, list) == 0)
		{
			// after the command, it has the data port number info only
			ptr = strtok(NULL, delim);
			memset(dataPortBuff, '\0', sizeof(dataPortBuff));	// Clear out the buffer
			strcpy(dataPortBuff, ptr);
			dataPortNum = atoi(dataPortBuff);
		}
		// handle the case where the command is '-g'
		else if (strcmp(ftCommand, trans) == 0)
		{
			// after the command, it has the file name info
			ptr = strtok(NULL, delim);
			memset(fileName, '\0', sizeof(fileName));		// Clear out the buffer
			strcpy(fileName, ptr);
			// then, it also contains the data port number info
			ptr = strtok(NULL, delim);
			memset(dataPortBuff, '\0', sizeof(dataPortBuff));	// Clear out the buffer
			strcpy(dataPortBuff, ptr);
			dataPortNum = atoi(dataPortBuff);
		}

		// call a function to get names of files and directories inside ftclient's current default directory
		fileCount = listFiles(fileList);
		
		// if the command is '-g', check if the requested file is in ftserver's directory
		if (strcmp(ftCommand, trans) == 0)
		{
			fileExist = 0;		// clear the flag
			
			// loop through the list of file (and directory) names
			for (i = 0; i < fileCount; i++)
			{	
				// if the requested file exists, set the flag to 1
				if (strcmp(fileList[i], fileName) == 0)
				{
					fileExist = 1;
				}
			}
			// if the requested file is not in the directory, display the error message and send the negative message
			// to ftclient
			if (fileExist == 0)
			{
				printf("File \"%s\" requested on port %d.\n", fileName, dataPortNum);
				printf("File not found. Sending error message to %s:%d\n", hostName, portNumber);
				sendMessage(establishedConnectionFD, noFileMsg);
			}
			// if the requested file exists, send the positive message to ftclient
			else if (fileExist == 1)
			{
				printf("File \"%s\" requested on port %d.\n", fileName, dataPortNum);
				sendMessage(establishedConnectionFD, yesFileMsg);
			}
		}

		// if the flag value is 1, proceed with opening data connection
		if (fileExist == 1)
		{
			// Set up the server address struct
			memset((char*)&clientAddress, '\0', sizeof(clientAddress));	// Clear out the address struct
			clientAddress.sin_family = AF_INET;				// Create a network-capable socket
			clientAddress.sin_port = htons(dataPortNum);			// Store the port number
			clientHostInfo = gethostbyname(hostName);			// Convert the machine name into a special form of address

			if (clientHostInfo == NULL)		// Error checking for the host name conversion
			{
				fprintf(stderr, "Server: ERROR, no such host\n");
				exit(0);
			}

			// Copy in the address
			memcpy((char*)&clientAddress.sin_addr.s_addr, (char*)clientHostInfo->h_addr, clientHostInfo->h_length);

			// Set up the socket
			socketFD = socket(AF_INET, SOCK_STREAM, 0);		// Create the socket
			if (socketFD < 0)					// Error checking for the socke creation
			{
				error("Server: ERROR opening socket");
			}

			// Call a function to connect to ftclient. If connection is successful, proceed with handling the request
			if (initContact(socketFD, clientAddress) == 1)
			{
				// call a function to handle the file request sent from ftclient
				handleRequest(socketFD, ftCommand, fileList, fileCount, fileName, hostName, dataPortNum);

				printf("\nData connection with ftclient closed.\n");
			}

			// close the data socket after handling the request
			close(socketFD);
		}

		// free the memory allocated for the list of file (and directory) names
		for (i = 0; i < fileCount; i++)
		{
			free(fileList[i]);
		}
	}
}
