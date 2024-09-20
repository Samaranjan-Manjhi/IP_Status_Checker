IP Status Checker

Overview

This project implements a simple server-client architecture that allows clients to check the status of an IP address using ICMP Echo Requests (ping). The server listens for incoming connections and responds with the status of the requested IP address (active or inactive).
Features

    Server: Listens for client connections and checks the status of the provided IP address.
    Client: Connects to the server and sends IP addresses for status checking.
    Signal Handling: Both server and client handle Ctrl + C for graceful termination.

Requirements

    C++11 or later
    A Unix-like environment (Linux or macOS)
    Sudo privileges may be required to run the server due to raw socket usage.


Usage
   Running the Server
   Start the server by specifying a port number (e.g., 12345):
   ./server 12345

   Running the Client
   In a new terminal, run the client by specifying the server's IP address and the port number:
   ./client <Server_IP_Address> 12345 ...Server IP Address where ./server binary is executed that system ip address

   Replace <Server_IP_Address> with the actual IP address of the server.
   
   Checking IP Status
   Once the client is connected, you can enter IP addresses to check their status. Press Ctrl + C to exit the client gracefully.
