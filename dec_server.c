#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define MAX_SIZE  65536

// Error function used for reporting issues
void error(const char *msg, int errno) {
  fprintf(stderr, "%s", msg);
  exit(errno);
} 

int convertToInt (char c) {

  int value = 0;

  if ( c == ' ')
    value = 26;
  else
    value = c - 'A';  // character arithmetic for ascii value

  return value;
}

char convertToChar(int i) {
  char c;

  if (i == 26)
    c = ' ';
  else 
    c = i + 'A';
  
  return c;
}

void decrypt(char *msg, char *key) {

  int i = 0;
  char temp;

  while (msg[i] != '\n') {
    
    // convert to ascii code and encrypt
    temp = (convertToInt(msg[i]) - convertToInt(key[i]));
    if (temp < 0){
      temp += 27;
    }
    msg[i] = convertToChar(temp);
    i++;
  }
  msg[i] = '\0';

}

void checkBackgroundPids()
{
    pid_t pid;
    int childStatus;
    // Check for terminated child background processes.    
    while ((pid = waitpid(-1, &childStatus, WNOHANG)) > 0){}
}

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);
  // Allow a client at any address to connect to this server
  address->sin_addr.s_addr = INADDR_ANY;
}

int main(int argc, char *argv[]){
  int connectionSocket, charsRead, status;
  char buffer[MAX_SIZE], message[MAX_SIZE], key[MAX_SIZE];
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);

  // Check usage & args
  if (argc < 2) { 
    fprintf(stderr,"USAGE: %s port\n", argv[0]); 
    exit(1);
  } 
  
  // Create the socket that will listen for connections
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    error("ERROR opening socket\n", 2);
  }

  // Set up the address struct for the server socket
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // Associate the socket to the port
  if (bind(listenSocket, 
          (struct sockaddr *)&serverAddress, 
          sizeof(serverAddress)) < 0){
    error("ERROR on binding\n", 2);
  }

  // Start listening for connetions. Allow up to 5 connections to queue up
  listen(listenSocket, 5); 
  
  // Accept a connection, blocking if one is not available until one connects
  while(1){

    checkBackgroundPids();

    // Accept the connection request which creates a connection socket
    connectionSocket = accept(listenSocket, 
                (struct sockaddr *)&clientAddress, 
                &sizeOfClientInfo); 
    if (connectionSocket < 0){
      error("ERROR on accept\n", 2);
    }

    pid_t pid = fork();

    switch(pid){
      case -1:{
        error("failed to fork process!\n", 2);
      }
      case 0:{

        //printf("SERVER: Connected to client running at host %d port %d\n", 
                          // ntohs(clientAddress.sin_addr.s_addr),
                          // ntohs(clientAddress.sin_port));

        // Get identity of client and validate
        memset(buffer, '\0', sizeof(buffer));
        // Read the client's identity message from the socket
        charsRead = recv(connectionSocket, buffer, sizeof(buffer) - 1, 0);
        if (charsRead < 0){
          error("ERROR reading from socket\n", 2);
        } 
        if (strcmp(buffer, "OTP_DECRYPT") != 0){
          charsRead = send(connectionSocket, "INVALID", 7, 0);
          exit(2);
        } else {

          memset(buffer, '\0', sizeof(buffer));
          charsRead = send(connectionSocket, "VALID", 5, 0);        
        
        // Get the message from the client and display it
        memset(buffer, '\0', sizeof(buffer));
        // Read the client's message from the socket
        charsRead = recv(connectionSocket, buffer, sizeof(buffer) - 1, 0); 
        if (charsRead < 0){
          error("ERROR reading from socket\n", 2);
        }
        //printf("SERVER: I received this from the client: \"%s\"\n", buffer);
        strcpy(message, buffer);

        // Get the key from the client and display it
        memset(buffer, '\0', sizeof(buffer));
        // Read the client's secret key from the socket
        charsRead = recv(connectionSocket, buffer, sizeof(buffer) - 1, 0); 
        if (charsRead < 0){
          error("ERROR reading from socket\n", 2);
        }
        //printf("SERVER: I received this key the client: \"%s\"\n", buffer);
        strcpy(key, buffer);

        decrypt(message, key);

        // Send encrypted message back to the client
        charsRead = send(connectionSocket, message, sizeof(message), 0);
        if (charsRead < 0){
          error("ERROR writing to socket\n", 2);
        }
        exit(0);
      }
      }
      default:{

        waitpid(pid, &status, WNOHANG); 
      }
      
    }
    // Close the connection socket for this client
    close(connectionSocket); 
  }
  // Close the listening socket
  close(listenSocket); 
  return 0;
}
