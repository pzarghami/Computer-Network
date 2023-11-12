#include <bits/stdc++.h> 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
	
#define SENDER_ACK_SIZE 10
#define SEQ_PACKET_NUM 6 
#define PACKET_NUM 680
#define PORT	 8080
#define MAXLINE  2048
#define BASE 0
int packetNum = PACKET_NUM;

const int CMD_IN_LEN = 1024; // Maximum command length received in telecommunications. 
const char* IP_ADDRESS = "127.0.0.1"; // The address 127.0. 0.1 is the standard address for IPv4 loopback traffic.
const char* CLIENT_SERVER_ERR = "Error in connecting to server\n";

int sockfd;
struct sockaddr_in servaddr, cliaddr;
socklen_t len;
vector<bool> isPacketLost;
string packets = "";
string extractedPID = ""; // packer ID that is extracted
int pc_num;
int fdRouter;

void randomPacketLost(int packetNum){
	for(int i = 0 ; i < packetNum/10 ; i++)
		isPacketLost[rand()%(packetNum)] = true;
}

int PIDextractor(char * buffer){
	string buff = string(buffer);
	return stoi(buff.substr(BASE, SEQ_PACKET_NUM));
}

string convertIntToString(int number){
    std::stringstream ss;
    ss << std::setw(SEQ_PACKET_NUM) << std::setfill('0') << to_string(number);
    return ss.str();
}

void writeToFile(){
	ofstream writeFile("sendFile.txt");
	writeFile << packets;
	writeFile.close();
}

void recievePacket(int packetNum){
	char buffer[MAXLINE];
	buffer[MAXLINE] = '\0';

	int id = 0;
    while(id < packetNum){
        recv(fdRouter, buffer, MAXLINE, 0);
		
		id = PIDextractor(buffer);
		char ack[SENDER_ACK_SIZE];
		strcpy(ack, ("ACK " + convertIntToString(id)).c_str());
		cout << "ACK :: " << ack << endl;
		if(isPacketLost[id]){
			isPacketLost[id] = false;
		}
		else{
		    send(fdRouter, ack, strlen(ack), 0);
		}
		id ++;
	}
    pthread_exit(0);
}

int connect_server(int port) 
{
    int fd;
    struct sockaddr_in server_address;
    char buff[CMD_IN_LEN] = {0};
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
    
    server_address.sin_family = AF_INET; 
    server_address.sin_port = htons(port); 
    server_address.sin_addr.s_addr = inet_addr(IP_ADDRESS);


    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)  // checking for errors
        printf("%s", CLIENT_SERVER_ERR);

    recv(fd, buff, CMD_IN_LEN, 0);
    pc_num = atoi(buff);
    printf("Client name is: %d\n", pc_num);
    
    return fd;
}

int main() {

    int fd = connect_server(PORT);
    fdRouter = fd;

	vector<bool> tempPacket(packetNum, 0);
	isPacketLost = tempPacket;

	// randomPacketLost(packetNum);
	while (true)
	{
	    char buff[CMD_IN_LEN] = {0};

		sprintf(buff, "PCB\n");
        send(fd, buff, strlen(buff), 0);

		recievePacket(packetNum);

	}
	
	return 0;
}