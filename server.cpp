#include <netinet/ip_icmp.h>
#include <bits/stdc++.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <string>
#include <chrono>
#include <thread>
#include <cstdio>
#include <atomic>
#include <ctime>
using namespace std;
#define PACKET_SIZE 64

volatile atomic<bool> stop_flag(false);
void signal_handler(int signal) 
{
    cout << "Stopping the program..." << endl;
    stop_flag.store(true);
}

bool sendStatus(int sockfd, bool status) 
{
    string response = (status) ? "Active" : "Inactive";
    ssize_t bytesSent = send(sockfd, response.c_str(), response.length(), 0);
    if (bytesSent <= 0) 
    {
        cerr << "Error sending response to client!" << endl;
        return false;
    }
    return true;
}

unsigned short calculateChecksum(unsigned short* buffer, int length) 
{
    unsigned long sum = 0;
    while (length > 1) {
        sum += *buffer++;
        length -= 2;
    }
    if (length == 1) {
        sum += *(unsigned char*)buffer;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

string getSelfIP() 
{
    struct ifaddrs* ifAddrStruct = NULL;
    struct ifaddrs* ifa = NULL;
    void* tmpAddrPtr = NULL;
    string selfIP = "";
    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) 
    {
        if (!ifa->ifa_addr) 
        {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) 
        {
            tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if (strcmp(ifa->ifa_name, "lo") != 0) 
            {
                selfIP = addressBuffer;
                break;
            }
        }
    }
    if (ifAddrStruct != NULL) 
    {
        freeifaddrs(ifAddrStruct);
    }
    return selfIP;
}

bool isIPActive(const string& ipAddress) 
{
    // Create raw socket for ICMP communication
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0) 
    {
        cerr << "Error creating socket" << endl;
        return false;
    }
    int optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    char packet[PACKET_SIZE];
    memset(packet, 0, sizeof(packet));
    // Set up the ICMP header
    icmp* icmpHeader = (icmp*)packet;
    icmpHeader->icmp_type = ICMP_ECHO;
    icmpHeader->icmp_code = 0;
    icmpHeader->icmp_id = htons(getpid());
    icmpHeader->icmp_seq = htons(1);
    icmpHeader->icmp_cksum = 0;
    icmpHeader->icmp_cksum = calculateChecksum((unsigned short*)icmpHeader, sizeof(icmp));
    sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = 0;
    inet_pton(AF_INET, ipAddress.c_str(), &(destAddr.sin_addr));
    // Send the ICMP Echo Request packet
    int bytesSent = sendto(sock, packet, sizeof(icmp), 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
    if (bytesSent < 0) 
    {
        cerr << "Error sending packet" << endl;
        return false;
    }
    // Receive the ICMP Echo Reply packet
    char recvBuffer[PACKET_SIZE];
    sockaddr_in senderAddr;
    socklen_t senderLen = sizeof(senderAddr);
    int bytesReceived = recvfrom(sock, recvBuffer, sizeof(recvBuffer), 0, (struct sockaddr*)&senderAddr, &senderLen);
    if (bytesReceived < 0) 
    {
        cerr << "Error receiving packet" << endl;
        return false;
    }
    // Parse the received packet to extract information (e.g., round-trip time, TTL, etc.)
    iphdr* ipHeader = (iphdr*)recvBuffer;
    icmp* recvIcmpHeader = (icmp*)(recvBuffer + (ipHeader->ihl << 2));
    if (strcmp(ipAddress.c_str(), getSelfIP().c_str()) == 0 || strcmp(ipAddress.c_str(), "127.0.0.1") == 0) 
    {
        return true;
    } 
    else if (recvIcmpHeader->icmp_type == ICMP_ECHOREPLY) 
    {
        return true;
    } 
    else 
    {
        return false;
    }
    // Close the socket
    close(sock);
}

void handleClient(int clientSockfd) 
{
    char clientIP[INET_ADDRSTRLEN];
    sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    getpeername(clientSockfd, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientAddrLen);
    inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
    cout << "Connected to client " << clientIP << endl;
    string message;
    while (!stop_flag.load()) 
    {
        char buffer[1024];
        ssize_t bytesRead = recv(clientSockfd, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) 
        {
            cerr << "Error receiving message from client!" << endl;
            break;
        }
        message = string(buffer, bytesRead);
        cout << "Received message from client " << clientIP << ": " << message << endl;
        bool activeStatus = isIPActive(message);
        if (!sendStatus(clientSockfd, activeStatus)) 
        {
            cerr << "Error sending status to client!" << endl;
            break;
        }
    }
    close(clientSockfd);
    cout << "Connection closed with client " << clientIP << endl;
}


int main(int argc, char* argv[]) 
{
    if (argc < 2) 
    {
        cerr << "Enter First these details: " << argv[0] << " <Port Number>" << endl;
        return 1;
    }
    // Set up signal handling
    signal(SIGINT, signal_handler);
    int portNumber = stoi(argv[1]);
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        cerr << "Error creating socket!" << endl;
        return 1;
    }
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNumber);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) 
    {
        cerr << "Error binding socket!" << endl;
        close(sockfd);
        return 1;
    }
    if (listen(sockfd, SOMAXCONN) < 0) 
    {
        cerr << "Error listening on socket!" << endl;
        close(sockfd);
        return 1;
    }
    cout << "Waiting for incoming connections... Just Wait Please" << endl;
    while (!stop_flag.load()) 
    {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSockfd = accept(sockfd, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientAddrLen);
        if (clientSockfd < 0) 
        {
            cerr << "Error accepting client connection!" << endl;
            continue;
        }
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
        cout << "Connected to client " << clientIP << endl;
        // Create a new thread to handle the communication with the client
        thread clientThread(handleClient, clientSockfd);
        clientThread.detach();
    }
    close(sockfd);
    return 0;
}

