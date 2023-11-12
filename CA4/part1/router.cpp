#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>
#include <unistd.h>
#include <bits/stdc++.h> 

using namespace std;
	
#define PORT1	 8080
#define PORT2	 8081
#define MAXLINE  2048
#define SEQ_PACKET_NUM 6 
#define BASE 0
#define BASE_ACK 4
#define PROPER_TIME 0.01 // proper time to send(router delay to sink data)
#define MAX_SAVING 10
#define DELAY 1

int sockfd1, sockfd2;
struct sockaddr_in servaddr1, cliaddr1;
struct sockaddr_in servaddr2, cliaddr2;
socklen_t len1,len2;

vector<bool> isPacketLost;
vector<bool> isAckPacketLost;

char buffer[MAXLINE];
vector<string> packets;

vector<string> savingAtoB;
vector<string> savingBtoA;

void randomPacketLost(vector<bool>& PacketLost, int packetNum){
	for(int i = 0 ; i < packetNum/20 ; i++)
		PacketLost[rand()%(packetNum)] = true;
}

int PIDextractor(char * buffer, int base){
	string buff = buffer;
	return stoi(buff.substr(base, SEQ_PACKET_NUM));
}

void print(vector<string> q)
{
	cout << "*********" << endl;
    for(int i = 0 ; i < q.size() ; i++)
		cout << stoi(q[i].substr(BASE, SEQ_PACKET_NUM)) << endl;
    cout << '\n';
	cout << "THE END" << endl;
}

void* recievePacketFromA(void* input){
	int id = 0;
    intptr_t packetNum = (intptr_t)input;
	while(id < packetNum){
		char buff[MAXLINE];
		while(savingAtoB.size() == MAX_SAVING/2);
		recvfrom(sockfd1, (char*)buff, MAXLINE, MSG_WAITALL, (struct sockaddr*) &cliaddr1, (socklen_t*)&len1);
		id = PIDextractor(buff, BASE);
		savingAtoB.push_back(string(buff));
		cerr << "From A " <<  id << endl;
		id ++;
		sleep(PROPER_TIME);
	}
    pthread_exit(0);
}

void* sendPacketToB(void* input){
    int id = 0;
	sleep(PROPER_TIME);
    intptr_t packetNum = (intptr_t)input;
	while(id < packetNum){
		while(savingAtoB.empty());
		sleep(DELAY);
		char buff[2048];
		strcpy(buff, savingAtoB[0].c_str());
		savingAtoB.erase(savingAtoB.begin());
		id =  PIDextractor(buff, BASE);
		if(isPacketLost[id]){
			isPacketLost[id] = false;
			cout << "Packet "<< id << " lost"<< endl;
		}
		else{
			sendto(sockfd2, buff, strlen(buff), MSG_CONFIRM, (const struct sockaddr*) &servaddr2, len2);
			cerr << "To B: " << id  << endl;
		}
		id ++;
	}
    pthread_exit(0);
}

void* recievePacketFromB(void* input){
    intptr_t packetNum = (intptr_t)input;
	int id = 0;
    while(id < packetNum){
		char ack[10];
		while(savingBtoA.size() == MAX_SAVING/2);
		recvfrom(sockfd2, ack, MAXLINE, MSG_WAITALL, (struct sockaddr*) &servaddr2, (socklen_t*)&len2);
		id = PIDextractor(ack, BASE_ACK);
		cerr << "FromB " << id << endl;
		savingBtoA.push_back(string(ack));
		id ++;
		sleep(PROPER_TIME);
	}
    pthread_exit(0);
}

void* sendPacketToA(void* input){
    intptr_t packetNum = (intptr_t)input;
	int id = 0;
    while(id < packetNum){
		char ack[10];
		while(savingBtoA.empty());
		strcpy(ack, savingBtoA[0].c_str());
		id =  PIDextractor(ack, BASE_ACK);
		savingBtoA.erase(savingBtoA.begin());
		if(isPacketLost[id]){
			isPacketLost[id] = false;
			cout << "Packet "<< id << " lost"<< endl;
		}
		else{
			sendto(sockfd1, ack, strlen(ack), MSG_CONFIRM, (const struct sockaddr*) &cliaddr1, len1);
			cerr << "To A: " << id  << endl;
		}
		id ++;
	}
    pthread_exit(0);
}


int main() {
		
	if ((sockfd1 = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

    if ((sockfd2 = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	
	memset(&servaddr1, 0, sizeof(servaddr1));
	memset(&cliaddr1, 0, sizeof(cliaddr1));

    memset(&servaddr2, 0, sizeof(servaddr2));
	memset(&cliaddr2, 0, sizeof(cliaddr2));
		
	servaddr1.sin_family = AF_INET; 
	servaddr1.sin_addr.s_addr = INADDR_ANY;
	servaddr1.sin_port = htons(PORT1);

    servaddr2.sin_family = AF_INET; 
	servaddr2.sin_addr.s_addr = INADDR_ANY;
	servaddr2.sin_port = htons(PORT2);
		
	if (bind(sockfd1, (const struct sockaddr *)&servaddr1, sizeof(servaddr1)) < 0){
		perror("bind1 failed");
		exit(EXIT_FAILURE);
	}

	len1 = sizeof(cliaddr1); 
    len2 = sizeof(cliaddr2); 
	buffer[2048] = '\0';

	recvfrom(sockfd1, (char*)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr*) &cliaddr1, (socklen_t*)&len1);
	int packetNum = (int)atoi(buffer);
	sendto(sockfd2, buffer, strlen(buffer), MSG_CONFIRM, (const struct sockaddr*) &servaddr2, len2);

	vector<bool> tempPacket(packetNum, 0);
	isPacketLost = tempPacket;
	isAckPacketLost = tempPacket;

	randomPacketLost(isPacketLost, packetNum);
	randomPacketLost(isAckPacketLost, packetNum);

	pthread_t sendToA;
    pthread_t recieveFromB;
	pthread_t sendToB;
    pthread_t recieveFromA;
    int threadnumber = (intptr_t) packetNum;
    pthread_create(&sendToA, NULL, recievePacketFromA, (void*)(intptr_t)threadnumber);
    pthread_create(&recieveFromB, NULL, sendPacketToB, (void*)(intptr_t)threadnumber);
	pthread_create(&sendToB, NULL, recievePacketFromB, (void*)(intptr_t)threadnumber);
    pthread_create(&recieveFromA, NULL, sendPacketToA, (void*)(intptr_t)threadnumber);

    pthread_join(sendToA, NULL);
    pthread_join(recieveFromB, NULL);	
    pthread_join(sendToB, NULL);
    pthread_join(recieveFromA, NULL);
		
	return 0;
}
