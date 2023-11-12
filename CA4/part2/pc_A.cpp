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
#include <iostream>
#include <vector>

#include <chrono>
#include <sys/time.h>
#include <ctime>
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::system_clock;

using namespace std;

#define PPC 34 // packets per client (count of packets that client must send) --> 680(count of packets) / 20(count of computers)
#define SW 5 //sliding window size
#define SENDER_ACK_SIZE 10
#define STDBY_TIME 0.001
#define PACKET_SIZE 1536
#define SEQ_PACKET_NUM 6 
#define PORT     8080
#define CLI_PORT 8085
#define PACKET_NUM 34
#define MAXLINE 1024
#define spanTime 6
#define BASE 4
const char* IP_ADDRESS = "127.0.0.1"; // The address 127.0. 0.1 is the standard address for IPv4 loopback traffic.
const int CMD_IN_LEN = 1024; // Maximum command length received in telecommunications. 
const char* CLIENT_SERVER_ERR = "Error in connecting to server\n";

int packetNum = 680;

bool windowReadyToSend[SW] = {0};
int pc_num;

vector <bool> AckReceived;
vector <time_t> sendTime;
vector <string> packets;

int sockfd; 
struct sockaddr_in servaddr; 
socklen_t len; 
int lastPacketWaitForAck = 0;
bool endSending = false;
bool changing = true;
int seq; // iterator for sendPacket or resend
int startID, finishID;
int fdRouter;

bool checkIdxSpan(int point)
{
    return (point >= startID) && (point < finishID);
}

int findLastPacketWaitForAck(){
    int index;
    for(int i = startID ; i < finishID ; i++)
        if(sendTime[i] && !AckReceived[i])
            return i;

    for(int i = finishID-1 ; i >= startID ; i--)
        if(AckReceived[i])
            return i+1;
    return finishID - 1;
}

string convertIntToString(int number){
    std::stringstream ss;
    ss << std::setw(SEQ_PACKET_NUM) << std::setfill('0') << to_string(number);
    return ss.str();
}

int PIDextractor(char * buffer, int base){
	string buff = string(buffer);
	return stoi(buff.substr(base, SEQ_PACKET_NUM));
}

void splitInput(int pc_num){
	string myText;
	string wholeText = "";
	ifstream myReadFile("t.txt");

	while (getline (myReadFile, myText))
		wholeText += (myText);
    
	myReadFile.close();

	string temp; //1530 = 1024 + 1024/2 - 6
    int count = (wholeText.size() % (PACKET_SIZE-SEQ_PACKET_NUM)) ? 1 : 0;
	for(int i = 0 ; i < (wholeText.size() / (PACKET_SIZE-SEQ_PACKET_NUM)) + count ; i++){
        temp = convertIntToString(i);
        temp += wholeText.substr(i*(PACKET_SIZE-SEQ_PACKET_NUM), (PACKET_SIZE-SEQ_PACKET_NUM));
        packets.push_back(temp); 
    }

}

void* sendPacket(void* input){
    char sendPacket[1600];
    for(seq = startID ; (seq < finishID) && (checkIdxSpan(seq)) ; seq++){
        while(windowReadyToSend[seq % SW]);
        sleep(1);
        memset(&sendPacket, 0, strlen(sendPacket));
        strcpy(sendPacket, packets[seq].c_str());
        send(fdRouter, sendPacket, strlen(sendPacket), 0);
        cout << "sendPacket number: " << PIDextractor(sendPacket, 0) << " " << windowReadyToSend[seq % SW] << endl;
        sendTime[seq] = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        windowReadyToSend[seq % SW] = true;
    }
    pthread_exit(0);
}

void* recievePacket(void* input){
    char ack[SENDER_ACK_SIZE];
    int id = startID;
    while(id < finishID && checkIdxSpan(id)){
        recv(fdRouter, ack, MAXLINE, 0);
        id = PIDextractor(ack, BASE);
        cout << "packet number " << id << " is: " << ack << endl;
        AckReceived[id] = true;
        windowReadyToSend[id % SW] = false;
        if(id == lastPacketWaitForAck)
            changing = true;
        lastPacketWaitForAck = findLastPacketWaitForAck();
        id ++;
    }
    endSending = true;
    pthread_exit(0);
}

void* measureTime(void* input){
    while(!endSending){
        while(changing && sendTime[lastPacketWaitForAck] && !AckReceived[lastPacketWaitForAck] &&
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count() - sendTime[lastPacketWaitForAck] >= spanTime){
            seq = findLastPacketWaitForAck();
            cout << "packet lost for comp " << pc_num << " : " << seq << endl;
            for(int i = 0 ; i < SW ; i++)
                windowReadyToSend[i] = false;
            changing = false;
        }   
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
    startID = (pc_num-1)*PACKET_NUM;
    finishID = startID + PACKET_NUM;
    
    return fd;
}

int main(int argc, char * argv[]) { 

    int fd = connect_server(PORT);
    fdRouter = fd;

    while(true)
    {
    	cout << "## we have to do this for one time: " <<endl; // corr 
        int client_socket, broadcast = 1, opt = 1;

        client_socket = socket(AF_INET, SOCK_DGRAM, 0);
        setsockopt(client_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
        setsockopt(client_socket, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

        struct sockaddr_in bc_address;

        bc_address.sin_family = AF_INET; 
        bc_address.sin_port = htons(CLI_PORT);
        bc_address.sin_addr.s_addr = inet_addr("255.255.255.255");//"192.168.1.255",IP_ADDRESS 

        bind(client_socket, (struct sockaddr *)&bc_address, sizeof(bc_address));        


        splitInput(pc_num);

        vector<time_t> tempTime(packetNum, 0);
        sendTime = tempTime;

        vector<bool> tempAckReceived(packetNum, 0);
        AckReceived = tempAckReceived;

        sleep(10);

        
        pthread_t send;
        pthread_t recieve;
        pthread_t time;
        int threadnumber = (intptr_t) packetNum;
        time_t begin = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        
        pthread_create(&send, NULL, sendPacket, (void*)(intptr_t)threadnumber);
        pthread_create(&recieve, NULL, recievePacket, (void*)(intptr_t)threadnumber);
        pthread_create( &time, NULL, measureTime, NULL);

        pthread_join(send, NULL);
        pthread_join(time, NULL);
        pthread_join(recieve, NULL);
        time_t end = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

        cout << "Time for sending all data and getting ACK: "<< end - begin << endl;
        break;
    }
    
    return 0; 
}