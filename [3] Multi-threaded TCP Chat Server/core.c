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
//#include <sched.h>
//#include <pthread.h>
#include "core.h"

#define PORT 8080

int awaitResponse (struct pollfd * serverSockets, int index, char * confirmationMessage) {
  int confirmation = poll(&serverSockets[index], 1, 30);
  int success = 0;
  if (confirmation < 0) {
    //error
  } else if (confirmation == 0 || (confirmation > 0 && serverSockets[index].revents == POLLHUP)) {
    //uhhh, server didn't respond in a timely manner/disconnected, drop connection.
    break;
  } else {
    //verify confirmation message
    success++;
  }
  serverSockets[index].revents = 0;
  return success;
}

int main(/*int argc, char* argv[]*/) {
  int unixCoreSocket;
  int tcpCoreSocket;
  int connectionLimit = 15; //number of logical cores -1. or not who cares anarchy 
  int denyConnection;
  int activeServers = 2;
  int readMessage;
  int yes = 1;
  int maxMessageLength = 2047;
  bool kill = false;
  int connectionAttempt = 0;
  char * message = malloc(sizeof(char) * maxMessageLength + 1); 

  //cpu_set_t logicalCores;
  //CPU_ZERO(&logicalCores);
  //CPU_SET(0, &logicalCores);
  //sched_setaffinity(0, sizeof(cpu_set_t), &logicalCores);

  struct sockaddr_un unixCoreAddress;
  socklen_t unixCoreAddressLen = sizeof(unixCoreAddress);
  unixCoreAddress.sun_family = AF_UNIX;
  strcpy(unixCoreAddress.sun_path, "/mnt/e/serverComm");
  unlink(unixCoreAddress.sun_path);

  struct unix_ip * addressList = malloc(sizeof(struct unix_ip) * connectionLimit);
  for (int i = 0; i < connectionLimit; i++) {
    addressList[i].ip = i;
    addressList[i].active = false;
  }

  if ((unixCoreSocket = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) { //creating socket on UNIX.
    perror("listening socket failed to make");
    exit(EXIT_FAILURE);
  }

  if (bind(unixCoreSocket, (struct sockaddr*)&unixCoreAddress, unixCoreAddressLen) < 0) { //binding to the port so the socket can do something
    perror("failed to bind to port");
    exit(EXIT_FAILURE);
  }

  if ((listen(unixCoreSocket, 16)) < 0) { //telling the socket to start listening for incomming connections
    perror("failed to listen");
    exit(EXIT_FAILURE);
  } 
  
  struct sockaddr_in tcpCoreAddress;
  socklen_t tcpCoreAddressLen = sizeof(tcpCoreAddress);
  tcpCoreAddress.sin_family = AF_INET;
  tcpCoreAddress.sin_addr.s_addr = INADDR_ANY;
  tcpCoreAddress.sin_port = htons(PORT);

  if ((tcpCoreSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) { //creating socket on TCP.
    perror("listening socket failed to make");
    exit(EXIT_FAILURE);
  }

  if (setsockopt(tcpCoreSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof(yes)) < 0) { //setting options for the host socket i.e. allowing multiple sockets on the same address and port
    perror("failed setting socket options"); 
    exit(EXIT_FAILURE);
  }

  if (bind(tcpCoreSocket, (struct sockaddr*)&tcpCoreAddress, unixCoreAddressLen) < 0) { //binding to the port so the socket can do something
    perror("failed to bind to port");
    exit(EXIT_FAILURE);
  }

  if ((listen(tcpCoreSocket, 16)) < 0) { //telling the socket to start listening for incomming connections
    perror("failed to listen");
    exit(EXIT_FAILURE);
  } 

  struct pollfd * serverSockets = malloc(sizeof(struct pollfd) * (connectionLimit + 2));
  for (int i = 0; i < connectionLimit + 2; i++) { //zeroing out the socket array
    serverSockets[i].fd = 0;
    serverSockets[i].events = 0;
    serverSockets[i].revents = 0;
  }

  serverSockets[0].fd = unixCoreSocket; 
  serverSockets[0].events = POLLIN | POLLHUP;

  serverSockets[1].fd = tcpCoreSocket;
  serverSockets[1].events = POLLIN | POLLHUP;

  while (!kill) {
    connectionAttempt = poll(serverSockets, connectionLimit + 2, 0); //TODO: set timeout for pinging servers.

    if (connectionAttempt < 0) { //there's an error
      perror("polling error occured\n");
      exit(EXIT_FAILURE);
    } else if (connectionAttempt == 0) { //no data was recieved.
      //TODO: Ping all the servers to see if they're online. If not disconnect them. 
    } else { //data was recieved, we need to figure out what actually happened.
      for (int i = 0; i < connectionLimit + 2; i++) {
        if (serverSockets[i].revents != 0) {
          if (i == 0) { //new server is trying to connect
            /*Accept the connection, update serverSockets to include the new connection, and start listening.
            All servers need to be made aware of the new server and told to open a connection to the new server.
            The new server needs to be assigned an address through which it can communicate with other servers.*/
            if (serverSockets[i].revents == POLLHUP) { //checking to see if a server is disconnecting. If it isn't we're good. If it is we need to handle it.
              //does this actually happen? Doesn't the disconnect signal come through the socket opened for the server? 
            } else if ((activeServers) < connectionLimit + 2) { //verifying we're under the connection limit.
              if((serverSockets[activeServers].fd = accept(unixCoreSocket, NULL, 0)) < 0) {
                perror("client socket failed to make");
                exit(EXIT_FAILURE);
              } else {
                serverSockets[activeServers].events = POLLIN | POLLHUP;
                serverSockets[i].revents = 0;
                //TODO: Assign the new server its IP. 
                for (int j = 0; j < connectionLimit; j++) {//checking for the first available IP
                  if (addressList[j].active == false) { 
                    char * ipString = malloc(sizeof(char) * 3);
                    snprintf(ipString, 3, "%d", addressList[j].ip);
                    char ipAssignment[21];
                    strcat(ipAssignment, "Your UNIX IP is: ");
                    strcat(ipAssignment, *ipString);
                    if (send(serverSockets[activeServers].fd, ipAssignment, 21, 0) < 0) { //telling the server its IP address.
                      //error lmao
                    } else { //waiting for confirmation that the server has its IP address using poll for a timeout.
                      char * confirmationMessage = malloc(sizeof(char) * 1); //TODO: size this buffer appropriately
                      if (awaitResponse(serverSockets, activeServers) == 1) {
                        //ayaya verify confirmation message
                      } else {
                        //error screaming in pain I want the end to come right now.
                        break;
                      }
                      free(confirmationMessage);
                    }
                    char ipCommunication[23];
                    strcat(ipCommunication, "New client has IP: ");
                    strcat(ipCommunication, *ipString);
                    for (int k = 2; k < activeServers; k++) { //telling all the existing servers that a new server is being added and what IP address it's being assigned. 
                      if (send(serverSockets[k].fd, ipCommunication, 23, 0) < 0) {
                        //error handling
                      } else {
                        //tbd
                      }
                    }
                    free(ipString);
                    addressList[j].active = true;
                  }
                  activeServers++;
                  break;
                }
              }
            } else { //we are not. deny the connection. 
              if((denyConnection = accept(unixCoreSocket, NULL, 0)) < 0) {
                perror("client socket failed to make");
                exit(EXIT_FAILURE);
              }
              //TODO: Accept the connection, tell the server to pound sand. Close connection.
              close(denyConnection);
            }
          } else if (i == 1) { //new client is trying to connect
          /*New client is trying to connect, we need to check the status of all servers.
          The first server with an available connection needs to connect to the client.
          All other servers need to do nothing. i.e. not just accept every connection comming in to the socket.
          The core needs to tell a chat server to accept a connection.*/

            for (int j = 2; j < activeServers; j++) {
              if (send(serverSockets[j].fd, penis, 12 inches, 0) < 0) {
                //error handling
              } else {
                char * confirmationMessage = malloc(sizeof(char) * 1); //TODO: Size this buffer
                if (awaitResponse() == 1) {
                  //server returned its availability, response needs to be parsed.
                } else {
                  //client disconnected. communicate that information to the people of the democratic peoples republic of korea but not really because I think that might be a federal crime.
                }
                free(confirmationMessage);
              }
            }

          } else if (i > 1) { //server is trying to communicate with the core

          } else { //handle error

          }
        }
      }
    }
  }

  free(addressList);
  free(message);
  free(serverSockets);
  return 0;
}