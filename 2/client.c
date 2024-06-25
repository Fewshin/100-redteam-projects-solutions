#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>

#define PORT 8080

int main (int argc, char* argv[]) {
  int clientSocket;
  int readMessage;
  int yes = 1;
  int maxMessageLength = 2047;
  bool kill = false;
  int connectionLimit = 16;
  int activeClients = 0;

  struct sockaddr_in hostAddress;
  socklen_t hostAddLen = sizeof(hostAddress);
  char * message = malloc(sizeof(char) * maxMessageLength + 1); 

  hostAddress.sin_family = AF_INET;
  hostAddress.sin_port = htons(PORT);
  if (inet_pton(AF_INET, "127.0.0.1", &hostAddr.sin_addr) <= 0) {
    perror("Invalid address/Address not supported.\n");
    exit(EXIT_FAILURE);
  }


  return 0;
}