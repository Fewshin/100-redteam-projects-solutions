# Chat Server Philosophy
## Primary Server/Core
- Takes new connections and passes them to chat servers with capacity.
- Keeps an index of active chat servers so they can communicate amoungst eachother.
- After communicating with the core, a chat server will be assigned an address in the UNIX IP space through which it will connect with every other chat server.
- Whenever the core accepts a new chat server, the core needs to tell every chat server there is a new connection and under which IP the new server will communicate.

## Chat Servers
- Each chat server would have a maximum number of clients.
- Any messages sent to the server would be transmitted to all of its clients as well as all other servers to be transmitted to their clients.

## Clients
- Communicate with the primary server to find chat servers with capacity.
- Connect to the chat server with capacity. 