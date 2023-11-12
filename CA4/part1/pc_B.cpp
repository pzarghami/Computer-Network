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
	
#define PORT	 8081
#define MAXLINE  2048
#define SEQ_PACKET_NUM 6 
#define BASE 0
#define SENDER_ACK_SIZE 10

int sockfd;
struct sockaddr_in servaddr, cliaddr;
socklen_t len;
vector<bool> isPacketRecieved;

string packets = "";
string extractedPID = ""; // packer ID that is extracted



int PIDextractor(char * buffer){
	string buff = buffer;
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
		recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &cliaddr, (socklen_t*)&len);
		// cout << buffer << endl;
		id = PIDextractor(buffer);
		// cout << id << endl;
		isPacketRecieved[id] = true; 
		char ack[SENDER_ACK_SIZE];
		strcpy(ack, ("ACK " + convertIntToString(id)).c_str());
		sendto(sockfd, ack, strlen(ack), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);
		cout << ack << endl;
		id ++;
	}
    pthread_exit(0);
}

int main() {
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
		
	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));
		
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(PORT);
		
	if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	len = sizeof(cliaddr);

	char buffer[10];
	buffer[10] = '\0';
	recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr*) &cliaddr, (socklen_t*)&len);
	int packetNum = (int)atoi(buffer);

	vector<bool> tempPacket(packetNum, 0);
	isPacketRecieved = tempPacket;
	

	recievePacket(packetNum);
	return 0;
}