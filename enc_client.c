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
void error(const char *msg) { 
  perror(msg); 
  exit(1); 
} 

int getNumBytes(const char *name){
  int numBytes = 0;

  FILE *file = fopen(name, "r");

  char c = fgetc(file);

  //TODO clean this up with DeMorgans Law
  while (1){
    
    if(c == EOF || c == '\n')
      break;

    if(!isupper(c) && c != ' ')
      error("enc_client error: input contains bad characters\n");

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
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
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
    fprintf(stderr,"USAGE: %s hostname plaintext key port\n", argv[0]); 
    exit(0); 
  } 

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    error("CLIENT: ERROR opening socket");
  }
  // Make the socket reusable
  int reuse = 1;
  setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
   // Set up the server address struct
  setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    error("CLIENT: ERROR connecting");
  }

  int fileBytes = getNumBytes(argv[1]);
  int keyBytes = getNumBytes(argv[2]);

  if(fileBytes > keyBytes) {
      fprintf(stderr, "Error: key '%s' is too short\n", argv[2]);
      exit(1);
  }



  int fd = open(argv[1], 'r');

  charsWritten = 0;

  while(charsWritten <= fileBytes){
    memset(buffer, '\0', sizeof(buffer));
    bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    charsWritten += send(socketFD, buffer, strlen(buffer), 0);
    memset(buffer, '\0', sizeof(buffer));
  }

  fd = open(argv[2], 'r');
  charsWritten = 0;

  while(charsWritten <= fileBytes){
    memset(buffer, '\0', sizeof(buffer));
    bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    charsWritten += send(socketFD, buffer, strlen(buffer), 0);
    memset(buffer, '\0', sizeof(buffer));
  }

  // Send message to server
  // Write to the server
  // charsWritten = send(socketFD, buffer, strlen(buffer), 0); 
  // if (charsWritten < 0){
  //   error("CLIENT: ERROR writing to socket");
  // }
  // if (charsWritten < strlen(buffer)){
  //   printf("CLIENT: WARNING: Not all data written to socket!\n");
  // }



  // Get return message from server
  // Clear out the buffer again for reuse
  memset(buffer, '\0', sizeof(buffer));
  // Read data from the socket, leaving \0 at end
  charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
  if (charsRead < 0){
    error("CLIENT: ERROR reading from socket");
  }
  //printf("CLIENT: I received this from the server: \"%s\"\n", buffer);
  printf("%s\n", buffer);

  // Close the socket
  close(socketFD); 
  return 0;
}