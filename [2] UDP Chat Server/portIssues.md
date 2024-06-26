# The crux of the problem:

Unless you bind a UDP socket to a particular port, it can randomly change ports. There are a couple of viable solutions such as:
- Only remembering the most recent port a communication came from meaning that only one client is available per IP address.
- Having the server assign a port to every single client on the same IP address. The client would then bind to that port. 

Ultimately, I have technically completed the challenge. There's just some UDP weirdness and I have chosen to implement the first solution because I value my time.