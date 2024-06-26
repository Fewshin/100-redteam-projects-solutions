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
#include <poll.h>

#define PORT 8080

int main (/*int argc, char* argv[]*/) {
  int clientSocket;
  int readMessage;
  //int yes = 1;
  int maxMessageLength = 2047;
  bool kill = false;
  int checkConnection = 0;
  int handshake;

  struct sockaddr_in hostAddress;
  socklen_t hostAddLen = sizeof(hostAddress);
  char * message = malloc(sizeof(char) * maxMessageLength + 1); 

  char connectionMessage[23] = "New client connected.\n";

  hostAddress.sin_family = AF_INET;
  hostAddress.sin_port = htons(PORT);
  if (inet_pton(AF_INET, "127.0.0.1", &hostAddress.sin_addr) <= 0) {
    perror("Invalid address/Address not supported.\n");
    exit(EXIT_FAILURE);
  }

  if ((clientSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { //making the socket
    perror("client socket failed to make\n");
    exit(EXIT_FAILURE);
  }

  if ((handshake = connect(clientSocket, (struct sockaddr*)&hostAddress, sizeof(hostAddress))) < 0) { //connecting because: https://stackoverflow.com/questions/9741392/can-you-bind-and-connect-both-ends-of-a-udp-connection
    perror("client failed to connect to host");
    exit(EXIT_FAILURE);
  }

  if (sendto(clientSocket, connectionMessage, sizeof(connectionMessage), 0, (struct sockaddr*)NULL, hostAddLen) < 0) {
    perror("failed to transmit data.\n");
    exit(EXIT_FAILURE);
  } else {
    //printf("server aware of our existence\n");
  }

  //TODO: connect to the server so it knows you want to recieve messages. 

  struct pollfd * fds = malloc(sizeof(struct pollfd) * 2);

  fds[0].fd = fileno(stdin); //creating pollfd for the console input
  fds[0].events = POLLIN;
  fds[0].revents = 0;

  fds[1].fd = clientSocket; //creating the pollfd for the socket input
  fds[1].events = POLLIN | POLLHUP;
  fds[1].revents = 0;

  while(!kill) {
    checkConnection = poll(fds, 2, 0); //polling for data

    if (checkConnection < 0) { //there's an error
      perror("polling error occured\n");
      exit(EXIT_FAILURE);
    } else if (checkConnection == 0) { //no data was recieved

    } else { //data was recieved, we need to figure out what actually happened.
      for (int i = 0; i < 2; i++) {
        if (fds[i].revents != 0) {
          if (i == 0) { //something in the stdin.
            if((readMessage = read(fileno(stdin), message, maxMessageLength + 1)) == 0) { //read the message, it it fails error out.
              //Not sure what this case is
            } else { //read the message and transmit it to the server
              message[readMessage] = '\0';
              message = realloc(message, readMessage+1);
              if (sendto(clientSocket, message, readMessage+1, 0, (struct sockaddr*)NULL, hostAddLen) < 0) {
                perror("failed to transmit data.\n");
                exit(EXIT_FAILURE);
              } else {
                //printf("message sent\n");
              }
              message = realloc(message, maxMessageLength + 1);
            }
          } else if (i == 1) { //something on the socket
            if (recvfrom(clientSocket, message, maxMessageLength, 0, (struct sockaddr*)NULL, &hostAddLen) < 0) { //reading incoming data on the socket. erroing out if something weird happens
              //error, why the hell did the recieve fail?
              perror("failed to recieve data.\n");
              exit(EXIT_FAILURE);
            } else { //display that data
              printf("%s\n", message);
            }
          } else {
            perror("for loop broken? world on fire? math doesn't make sense?\n");
            exit(EXIT_FAILURE);
          }
        }
        fds[i].revents = 0;
      }
    }
  }

  free(fds);
  return 0;
}