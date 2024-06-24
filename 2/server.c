#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/select.h>
//#include <sys/time.h>

#define PORT 8080

int main (int argc, char* argv[]) {
  int serverSocket, clientSocket;
  int readMessage;
  int yes = 1;
  int maxMessageLength = 2047;
  bool kill = false;
  int connectionLimit = 16;
  int activeClients = 0;

  struct sockaddr_in hostAddress;
  struct sockaddr_in incomingConnection;
  struct sockaddr_in ** clientAddress = malloc(sizeof(struct sockaddr_in*) * connectionLimit); 
  for (int i = 0; i < connectionLimit; i++) {
    *clientAddress[i] = NULL;
  }
  socklen_t hostAddLen = sizeof(hostAddress);
  char * message = malloc(sizeof(char) * maxMessageLength + 1); 

  hostAddress.sin_family = AF_INET;
  hostAddress.sin_addr.s_addr = INADDR_ANY;
  hostAddress.sin_port = htons(PORT);

  if ((serverSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("listening socket failed to make");
    exit(EXIT_FAILURE);
  }

  if (bind(serverSocket, (struct sockaddr*)&hostAddress, sizeof(hostAddress)) < 0) { //binding to the port so the socket can do something
    perror("failed to bind to port");
    exit(EXIT_FAILURE);
  }

  while (!kill) {
    if ((clientSocket = recvfrom()) < 0) {
      //error
      perror("failed to recieve data.\n");
      exit(EXIT_FAILURE);
    } else if (clientSocket == 0) {
      //disconnect
    } else {
      if (activeClients > connectionLimit - 1) {
        //dump oldest connection
      } else {
        for (int i = 0; i < activeClients; i++) {
          if (*clientAddress[i] == incomingConnection) {
            //struct sockaddr_in * holder = NULL;
            //holder = clientAddress[i];
            for (int j = i + 1; j < activeClients; j++) {
              clientAddress[j - 1] = clientAddress[j];
            }
            //clientAddress[activeClients] = holder;
            //activeClients--;
          }
        }
        clientAddress[activeClients] = malloc(sizeof(struct sockaddr_in));
        *clientAddress[activeClients] = incomingConnection;
        activeClients++;
      }

      //message
    }
  }

  for (int i = 0; i < connectionLimit; i++) {
    free(clientAddress[i]);
  }
  free(clientAddress)
  free(message);
  return 0;
}