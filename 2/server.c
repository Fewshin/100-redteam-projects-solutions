#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/select.h>

#define PORT 8080

int removeAddress (struct sockaddr_in ** clientList, struct sockaddr_in toBeRemoved, int clientCount) { // removes the first address with the same values as the toBeRemoved variable and slides the rest of the array over.
  for (int i = 0; i < clientCount; i++) {
    if (*clientList[i] == toBeRemoved) {
      free(clientList[i]);
      for (int j = i + 1; j < clientCount; j++) {
        clientList[j - 1] = clientList[j];
        clientList[j] = NULL;
      }
      return 1; //an address was removed.
    }
  }
  return 0; //an address failed to be removed.
}

int main (int argc, char* argv[]) {
  int serverSocket, clientSocket;
  int yes = 1;
  int maxMessageLength = 2047;
  bool kill = false;
  int connectionLimit = 16;
  int activeClients = 0;

  struct sockaddr_in hostAddress;
  struct sockaddr_in incomingConnection;
  struct sockaddr_in ** clientAddress = malloc(sizeof(struct sockaddr_in*) * connectionLimit); 
  for (int i = 0; i < connectionLimit; i++) { //Make the array of pointers an array of NULL pointers. 
    *clientAddress[i] = NULL;
  }
  socklen_t hostAddLen = sizeof(hostAddress);
  socklen_t incomingConnectionLen = sizeof(incomingConnection);
  char * message = malloc(sizeof(char) * maxMessageLength + 1); 

  hostAddress.sin_family = AF_INET;
  hostAddress.sin_addr.s_addr = INADDR_ANY;
  hostAddress.sin_port = htons(PORT);

  if ((serverSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("server socket failed to make");
    exit(EXIT_FAILURE);
  }

  if (bind(serverSocket, (struct sockaddr*)&hostAddress, sizeof(hostAddress)) < 0) { //binding to the port so the socket can do something
    perror("failed to bind to port");
    exit(EXIT_FAILURE);
  }

  while (!kill) {
    if ((clientSocket = recvfrom(serverSocket, message, sizeof(message), 0, (struct sockaddr*)&incomingConnection, &incomingConnectionLen)) < 0) { //client sees UDP connection and tries to intake data.
      //error, why the hell did the recieve fail?
      perror("failed to recieve data.\n");
      exit(EXIT_FAILURE);
    } else if (clientSocket == 0) { //this indicates that a UDP socket closed and is no longer interested in recieving data.
      //disconnect
      if (removeAddress(clientAddress, incomingConnection, activeClients) == 1) { //removing the disconnecting client from the list and reducing the active client count.
        activeClients--;
      } else {
        perror("tried to disconnect a client that's not in the client list.\n");
        exit(EXIT_FAILURE);
      }
    } else { //this indicates that a client is trying to connect and/or send a message.
      if (activeClients > connectionLimit - 1) { //checking if there's not enough room to blindly accept a new connection
        if (removeAddress(clientAddress, incomingConnection, activeClients) == 1) { //checking if it's a new message from an existing client. If so add it to the end of the array of active connections indicating that it was the IP that last transmitted data.
          clientAddress[activeClients] = malloc(sizeof(struct sockaddr_in));
          *clientAddress[activeClients] = incomingConnection;
        } else {
          if (removeAddress(clientAddress, *clientAddress[0], activeClients) == 1) { //It is a new client. We're full so we need to dump the oldest client and accept this new connection. 
            clientAddress[activeClients] = malloc(sizeof(struct sockaddr_in));
            *clientAddress[activeClients] = incomingConnection;
          } else { // Something weird has happened. We're full, we were unable to dump the oldest connection and the incoming connection is not a message from an existing client.
            perror("failed to dump oldest connection and accept new connection.\n");
            exit(EXIT_FAILURE);
          }
        }
      } else { //We have not hit the maximum number of connections. We can accept a new connection without dropping an old connection.
        if (removeAddress(clientAddress, incomingConnection, activeClients) == 1) { //checking if it's a connection from an existing client. If so append it to the end of the list as the most recent connection.
          clientAddress[activeClients] = malloc(sizeof(struct sockaddr_in)); 
          *clientAddress[activeClients] = incomingConnection;
        } else { //it's a new connection. We need to add it to the end of the list and increase the number of active clients. 
          clientAddress[activeClients] = malloc(sizeof(struct sockaddr_in)); 
          *clientAddress[activeClients] = incomingConnection;
          activeClients++;
        }
      }
      //fires off a message to every known client in the list. 
      message[clientSocket] = '\0';
      message = realloc(message, clientSocket+1);
      for (int i = 0; i < activeClients; i++) {
        if (sendto(serverSocket, message, clientSocket+1, 0, clientAddress[i], sizeof(clientAddress[i])) < 0) {
          perror("failed to transmit data.\n");
          exit(EXIT_FAILURE);
        }
      }
      message = realloc(message, maxMessageLength + 1);
    }
  }

  close(serverSocket);

  for (int i = 0; i < connectionLimit; i++) {
    free(clientAddress[i]);
  }
  free(clientAddress);
  free(message);
  return 0;
}