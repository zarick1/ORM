# SafeTransferProtocol

# Overview
SafeTransferProtocol is a UDP-based client-server application written in C, designed for reliable message transfer over a network. The client (sender) reads messages from a file (message.txt), sends them to the server (receiver) on port 8080, and waits for acknowledgments (ACKs). The server receives messages, logs them to receivedMessage.txt, and displays them using a ncurses-based terminal interface. The system includes retransmission logic to ensure reliable delivery.

# Features
Reliable UDP Transfer: Sends messages with unique IDs and retransmits up to 5 times if no ACK is received.
Message Format: Client sends 0.<id>.<size>.<data>, server responds with 1.<id>.ACK.
File Input/Output: Client reads messages from message.txt, server logs to receivedMessage.txt.
Ncurses Interface: Both client and server display status and messages in a terminal-based GUI.
Timeout Handling: Client waits 3 seconds for ACKs, server times out after 10 seconds of inactivity.
Error Handling: Validates message formats and handles terminal size, file, and socket errors.
Interrupt Support: Graceful exit on Ctrl+C with resource cleanup.

# Technologies
C: Core programming language.
UDP Sockets: For network communication.
Ncurses: For terminal-based graphical interface.
Linux: Target operating system.
Standard C Libraries: stdio.h, socket.h, time.h, etc.

# Project Structure
SafeTransferProtocol/
├── udp_server.c        # Server code (receiver)
├── udp_client.c        # Client code (sender)
├── build.sh            # Build script to compile both programs
├── message.txt         # Input file for client messages
├── receivedMessage.txt # Output file for server logs

# Prerequisites
Linux OS: Tested on Ubuntu or similar distributions.
GCC: For compiling C code.
Ncurses: For terminal interface.
Network Access: For UDP communication (default port 8080).

# Installation
Install ncurses:
sudo apt-get update
sudo apt-get install libncurses5-dev libncursesw5-dev


Clone the repository:
git clone https://github.com/zarickl/SafeTransferProtocol.git
cd SafeTransferProtocol


Build the programs:
chmod +x build.sh
./build.sh

This compiles udp_server.c into receiver and udp_client.c into sender.


# Usage
Prepare input file:

Create or edit message.txt in the project directory.
Add one message per line, e.g.:Test message 1
Test message 2
Test message 3

Run the server:

In one terminal, start the receiver:./receiver

The server listens on port 8080, displays incoming messages in a ncurses interface, and logs them to receivedMessage.txt.

Run the client:

In another terminal, start the sender:./sender

The client reads message.txt, sends each message to the server, and displays send/ACK status in a ncurses interface.

Monitor output:

Check receivedMessage.txt for logged messages.
Use the ncurses interfaces to monitor real-time status.
Press Ctrl+C to gracefully exit either program.

# Notes

Message Format: Messages must fit within 52 characters (defined by MAXCHAR). Longer messages may be truncated.
Retransmission: The client retransmits a message up to 5 times (MAXSEND) if no ACK is received within 3 seconds (TIMEOUT).
Terminal Size: Ensure the terminal is at least 65x9 characters for the client and meets server requirements (checked at runtime).
Local Testing: By default, the client connects to localhost (127.0.0.1). Modify servaddr.sin_addr.s_addr in udp_client.c for remote servers.
Improvements:
Add error handling for specific recvfrom errors beyond timeout.
Validate malloc return values in getAllPackages.
Close additional ncurses windows (e.g., messageWindow) in cleanUp.


