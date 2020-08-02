#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <ctype.h>

/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// Error function used for reporting issues
void error(const char *msg, int errno) { 
  fprintf(stderr, "%s", msg);
  exit(errno); 
} 

int getNumBytes(const char *name){
  int numBytes = 0;

  FILE *file = fopen(name, "r");
  if(file == NULL){
    error("CLIENT: Error opening specified file\n", 2);
  }
  

  char c = fgetc(file);

  while (1){
    
    if(c == EOF || c == '\n')
      break;

    if(!isupper(c) && c != ' ')
      error("dec_client error: input contains bad characters\n", 1);    
    numBytes++;
    c = fgetc(file);
  }
  fclose(file);

  return numBytes;
}

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber, 
                        char* hostname){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname(hostname); 
  if (hostInfo == NULL) { 
    error("CLIENT: ERROR, no such host\n", 2); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, 
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

int main(int argc, char *argv[]) {

  int socketFD, charsWritten, charsRead, bytesRead;
  struct sockaddr_in serverAddress;
  char buffer[256];
  // Check usage & args
  if (argc < 4) { 
    fprintf(stderr,"USAGE: %s hostname ciphertext key port\n", argv[0]); 
    exit(0); 
  } 

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    error("CLIENT: ERROR opening socket\n", 2);
  }
  // Make the socket reusable
  int reuse = 1;
  setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
   // Set up the server address struct
  setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    error("CLIENT: ERROR connecting\n", 2);
  }

  int fileBytes = getNumBytes(argv[1]);
  int keyBytes = getNumBytes(argv[2]);

  if(fileBytes > keyBytes) {
      fprintf(stderr, "Error: key '%s' is too short\n", argv[2]);
      exit(1);
  }

  //Send identity of client to server for validation
  char *id = "OTP_DECRYPT";
  charsWritten = send(socketFD, id, strlen(id), 0);
  memset(buffer, '\0', sizeof(buffer));
  if(charsWritten < 0){
    error("CLIENT: ERROR writing to socket\n", 2);
  }
  charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
  if(charsRead < 0) {
    error("CLIENT: ERROR reading from socket\n", 2);
  }
 
  if(strcmp(buffer, "INVALID") == 0){
    memset(buffer, '\0', sizeof(buffer));
    sprintf(buffer, "Error: could not contact enc_server on port %s\n", argv[3]);
    error(buffer, 2);
  }

   // Open file containing message
  int fd = open(argv[1], 'r');
  if(fd == -1){
    error("CLIENT: Could not open MESSAGE file\n", 2);
  }
  charsWritten = 0;
  // Send message to server
  // Write to the server
  while(charsWritten <= fileBytes){
    memset(buffer, '\0', sizeof(buffer));
    bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if(bytesRead < 0) {
      error("CLIENT: ERROR reading from FD\n", 2);
    }
    charsWritten += send(socketFD, buffer, strlen(buffer), 0);
    memset(buffer, '\0', sizeof(buffer));
  }
  close(fd);

   // Open file containing key
  fd = open(argv[2], 'r');
  if(fd == -1){
    error("CLIENT: Could not open KEY file\n", 2);
  }
  charsWritten = 0;
  // Send key to the server
  // Write to the server
  while(charsWritten <= fileBytes){
    memset(buffer, '\0', sizeof(buffer));
    bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if(bytesRead < 0) {
      error("CLIENT: ERROR reading from FD\n", 2);
    }
    charsWritten += send(socketFD, buffer, strlen(buffer), 0);
    memset(buffer, '\0', sizeof(buffer));
  }
  close(fd);

  // Get return message from server
  // Clear out the buffer again for reuse
  memset(buffer, '\0', sizeof(buffer));
  // Read data from the socket, leaving \0 at end
  charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
  if (charsRead < 0){
    error("CLIENT: ERROR reading from socket\n", 2);
  }
  //printf("CLIENT: I received this from the server: \"%s\"\n", buffer);
  printf("%s\n", buffer);

  // Close the socket
  close(socketFD); 
  return 0;
}