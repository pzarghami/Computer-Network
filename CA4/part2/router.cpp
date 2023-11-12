#include <bits/stdc++.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <vector>

using namespace std;
	
#define PROPER_TIME 0.00001 // proper time to send(router delay to sink data)
#define RANDOM_EARLY_SIZE 12
#define SEQ_PACKET_NUM 6 
#define PACKET_NUM 680
#define PORT1	 8080
#define PORT2	 8081
#define MAXLINE  1024
#define MAX_SAVING 20
#define BASE_ACK 4
#define BASE 0

struct sockaddr_in servaddr1, cliaddr1;
struct sockaddr_in servaddr2, cliaddr2;
int sockfd1, sockfd2;
socklen_t len1,len2;

vector<string> packets;
char buffer[MAXLINE];

deque<pair<string, int>> savingAtoB;
deque<pair<string, int>> savingBtoA;

int savedCountAtoB =0;
int savedCountBtoA =0;
int packetNum = PACKET_NUM;

int client_socket[1024] = {0}; // get client id by using socket
int client_id[25] = {0}; // get client socket
vector<bool> isPacketRecieveFromA;
vector<bool> isPacketRecieveFromB;
deque<pair<string, int>> saving;
int pc_B_ID;

int savedCount =0;

int PIDextractor(char * buffer, int base){
	string buff = buffer;
	return stoi(buff.substr(base, SEQ_PACKET_NUM));
}

bool getProbability(int size){
	srand(time(0));
	int prob = MAX_SAVING - size;
	if(rand()%prob == 0)
		return true;
	return false;
}

void randomEarlyDetection(){
	if(savingAtoB.size() + savingBtoA.size() > RANDOM_EARLY_SIZE){
		if(getProbability(savingAtoB.size() + savingBtoA.size() )){
			if(!savingAtoB.size())
				savingAtoB.erase(savingAtoB.begin());
			if(!savingBtoA.size())
				savingBtoA.erase(savingAtoB.begin());
		}
	}
}

void sendPacketToB(){
	if (!savedCountAtoB)
		return;
	
	sleep(PROPER_TIME);
	while(savedCountAtoB == MAX_SAVING/2)
		sleep(PROPER_TIME);

	strcpy(buffer, (savingAtoB.back().first).c_str());
	send(pc_B_ID, buffer, strlen(buffer), 0);

	savingAtoB.pop_back();
	savedCountAtoB--;
}

void sendPacketToA(){
	if (!savedCountBtoA)
		return;

	while(savedCountBtoA == MAX_SAVING/2)
		sleep(PROPER_TIME);
	char ack[10];
	strcpy(ack, (savingBtoA.back().first).c_str());
	send(client_id[(savingBtoA.back().second/34+1)], ack, strlen(ack),	0);
	savingBtoA.pop_back();
	savedCountBtoA--;
}

void recievePacketFromA(char* buffer){
	string buff = string(buffer);
	int id = PIDextractor(buffer, BASE);
	savingAtoB.push_front(make_pair(buff, id));
	savedCountAtoB++;
	cout << "A add to buff : " << id <<endl;
	sleep(PROPER_TIME);
	sendPacketToB();
}

void recievePacketFromB(char* ack){
	int id = 0;

	cout << "ack: " << ack << endl;
	id = PIDextractor(ack, BASE_ACK);
	
	string ack_ = string(ack);
	savingBtoA.push_front(make_pair(ack_, id));
	savedCountBtoA++;
	cout << "saving BtoA : " << savedCountBtoA << endl;
	sendPacketToA();	
}

char* integer_to_string(int num)
{
    char num_char_type[] = "0123456789";
    int palindrome = 0;
    char res[4];
    char* id;
    int i=0;
	// cout << "num : " <<  num << endl;

    while (num)
    {
        int digit = num%10;
        palindrome *= 10;
        palindrome += digit;
        num /= 10;
    }

    while (palindrome)
    {
        res[i++] = (num_char_type[(palindrome%10)]) + '\0';
        palindrome /= 10;
    }
    res[i++] = '\0';

    id = res;
	// cout << "id : " <<  id << endl;

    return id;
}

int acceptClient(int server_fd) 
{
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t*) &address_len);

    return client_fd;
}

int setupServer(int port) 
{
    struct sockaddr_in address;
    int server_fd;
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    
    listen(server_fd, 8);

    return server_fd;
}

int main() {
    int server_fd, new_socket, max_sd, client_count=0;
    fd_set master_set, working_set;
    char buffer[1024] = {0};
    char temp_buffer[1024] = {0};

    server_fd = setupServer(8080);

    FD_ZERO(&master_set);
    max_sd = server_fd;
    FD_SET(server_fd, &master_set);


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
		

	len1 = sizeof(cliaddr1); 
    len2 = sizeof(cliaddr2); 
	sleep(5);

	packetNum = MAX_SAVING;
	vector<bool> tempPacket(packetNum, 0);
	isPacketRecieveFromA = tempPacket;
	isPacketRecieveFromB = tempPacket;

	while (true) 
    {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);
        for (int i = 0; i <= max_sd; i++) 
        {
            if (FD_ISSET(i, &working_set)) 
            {                
                if (i == server_fd) 
                {  
                    // new clinet
                    new_socket = acceptClient(server_fd);
                    client_count++;
                    FD_SET(new_socket, &master_set);
                    if (new_socket > max_sd)
                        max_sd = new_socket;
                    char client_name[4];
					string name = to_string(client_count);
					strcpy(client_name, name.c_str());
                    client_id[client_count] = new_socket;
                    client_socket[new_socket] = client_count;
					cout << "client id : " << client_name << endl;
                    send(new_socket, client_name, strlen(client_name), 0);
                    printf("New client connected. fd = %d\n", new_socket);
                }
                
                else 
                { 
                    // client sending msg
                    int bytes_received;
                    bytes_received = recv(i , buffer, 1024, 0);
                    
                    if (bytes_received == 0) 
                    { // EOF
                        printf("client fd = %d, ID = %d closed\n", i, client_socket[i]);
                        close(i);
                        FD_CLR(i, &master_set);
                        continue;
                    }

                    else
                    {
						if (!strcmp(buffer, "PCB\n"))
						{
							client_count --;
							pc_B_ID = i;
			                client_id[client_socket[i]] = 0;
            		        client_socket[i] = 0;
						}
						
                        memset(temp_buffer, 0, 1024);
                        memcpy(temp_buffer, &buffer[0], 3);
                        temp_buffer[4] = '\0';
						if (!strcmp(temp_buffer, "ACK"))
						{
							recievePacketFromB(buffer);
						}
						memset(temp_buffer, 0, 1024);
                        memcpy(temp_buffer, &buffer[0], 3);
                        temp_buffer[4] = '\0';
						if((i != pc_B_ID) && (!strcmp(temp_buffer, "000")))
							recievePacketFromA(buffer);
                        // else
                        // {
                        //     write(1,"This command is not supported.\n", 40);
                        // }
                    }
                
                    // printf("client %d: %s\n", i, buffer);
                    printf("client %d: \n", i);
                    memset(buffer, 0, 1024);
                }
            }
        }
    }

	return 0;
}
