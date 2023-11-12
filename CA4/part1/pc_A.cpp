#include <stdio.h> 
#include <bits/stdc++.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <iostream>
#include <fstream>
#include <string>
#include <pthread.h>
#include <cstdint>

#include <chrono>
#include <sys/time.h>
#include <ctime>
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

using namespace std;

#define PORT     8080 
#define MAXLINE 1024
#define SW 5 //sliding window size
#define PACKET_SIZE 1536
#define SEQ_PACKET_NUM 6 
#define BASE 4
#define SENDER_ACK_SIZE 10
#define STDBY_TIME 0.001
#define spanTime 7500

bool windowReadyToSend[SW] = {0};

vector <string> packets;
vector <bool> AckReceived;
vector <time_t> sendTime;

int sockfd; 
struct sockaddr_in servaddr; 
socklen_t len; 
int lastPacketWaitForAck = 0;
bool endSending = false;

int seq; // iterator for sendPacket or resend
int packetNum;

int findLastPacketWaitForAck(){
    int index;
    for(int i = 0 ; i < packetNum ; i++)
        if(sendTime[i] && !AckReceived[i])
            return i;

    for(int i = packetNum-1 ; i >= 0 ; i--)
        if(AckReceived[i])
            return i+1;
    return packetNum - 1;
}

string convertIntToString(int number){
    std::stringstream ss;
    ss << std::setw(SEQ_PACKET_NUM) << std::setfill('0') << to_string(number);
    return ss.str();
}

int PIDextractor(char * buffer,int base = BASE){
	string buff = buffer;
	return stoi(buff.substr(base, SEQ_PACKET_NUM));
}

int splitInput(){
	string myText;
	string wholeText = "";
	ifstream myReadFile("t.txt");

	while (getline (myReadFile, myText))
		wholeText += (myText);
    
	string temp; //1530 = 1024 + 1024/2 - 6
    int count = (wholeText.size() % (PACKET_SIZE-SEQ_PACKET_NUM)) ? 1 : 0;
	for(int i = 0 ; i < (wholeText.size() / (PACKET_SIZE-SEQ_PACKET_NUM)) + count ; i++){
        temp = convertIntToString(i);
        temp += wholeText.substr(i*(PACKET_SIZE-SEQ_PACKET_NUM), (PACKET_SIZE-SEQ_PACKET_NUM));
        packets.push_back(temp); 
    }
	myReadFile.close();
    return packets.size();
}

void* sendPacket(void* input){
    char sendPacket[1600];
    for(seq = 0 ; seq < packetNum ; seq++){
        while(windowReadyToSend[seq % SW]);
        strcpy(sendPacket, packets[seq].c_str());
        cerr << "sendPacket number: " << PIDextractor(sendPacket, 0) << " " << windowReadyToSend[seq % SW] << endl;
        // cerr << packets[seq] << endl;
        sendto(sockfd, sendPacket, strlen(sendPacket), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
        sendTime[seq] = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        windowReadyToSend[seq % SW] = true;
    }
    pthread_exit(0);
}

void* recievePacket(void* input){
    char ack[SENDER_ACK_SIZE];
    int id = 0;
    while(id < packetNum){
        recvfrom(sockfd, (char*)ack, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, (socklen_t*)&len); 
        id = PIDextractor(ack);
        lastPacketWaitForAck = findLastPacketWaitForAck();
        cerr << "packet number " << id << " is: " << ack << endl;
        windowReadyToSend[id % SW] = false;
        AckReceived[id] = true;
        id ++;
    }
    endSending = true;
    pthread_exit(0);
}

void* measureTime(void* input){
    while(!endSending){
        while(sendTime[lastPacketWaitForAck] && !AckReceived[lastPacketWaitForAck] &&
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - sendTime[lastPacketWaitForAck] >= spanTime){
            seq = findLastPacketWaitForAck();
            cerr << seq << " lost..." << 
            duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - sendTime[lastPacketWaitForAck] << endl;
            sendTime[lastPacketWaitForAck] = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            for(int i = 0 ; i < SW ; i++)
                windowReadyToSend[i] = false;
        }   
    }
    pthread_exit(0);
}

int main() { 
    char buffer[MAXLINE]; 
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    memset(&servaddr, 0, sizeof(servaddr)); 
        
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_addr.s_addr = INADDR_ANY; 


    packetNum = splitInput();

    vector<time_t> tempTime(packetNum, 0);
	sendTime = tempTime;

    vector<bool> tempAckReceived(packetNum, 0);
    AckReceived = tempAckReceived;

    char sendPacketNum[100];
    string tempPacketNum = to_string(packetNum);;
    strcpy(sendPacketNum, tempPacketNum.c_str());
    sendto(sockfd, sendPacketNum, strlen(sendPacketNum), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));

    pthread_t send;
    pthread_t recieve;
    pthread_t time;
    int threadnumber = (intptr_t) packetNum;

    time_t begin = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    pthread_create( &send, NULL, sendPacket, (void*)(intptr_t)threadnumber);
    pthread_create( &recieve, NULL, recievePacket, (void*)(intptr_t)threadnumber);
    pthread_create( &time, NULL, measureTime, NULL);

    pthread_join(send, NULL);
    pthread_join(time, NULL);
    pthread_join(recieve, NULL);
    time_t end = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

    cout << "Time for sending all data and getting ACK: "<< end - begin << endl;

    
    close(sockfd); 
    return 0; 
}