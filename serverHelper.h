#include <cstdint>
#include <sched.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <ratio>
#include <mutex>
#include <ncurses.h>
#include <sstream>
#include <iomanip> 

using namespace std;

mutex clientMutex;

int sock = socket(AF_INET, SOCK_DGRAM, 0);
char ip [INET_ADDRSTRLEN] = "";
struct sockaddr_in address{};

uint64_t serverTime;
uint16_t num_clients;

struct p_msgs{
    char m[15];
    uint64_t t;
};

struct Client{
    sockaddr_in addr;
    int timeout;
    vector<p_msgs> pastMessages;
    uint16_t id;
    uint64_t server_offset;
} typedef Client ;

vector<Client> Clients;

string coutMessage;

char bufferRec[1024];

uint16_t generateUniqueId() {
    static uint16_t nextId = 1;

    while(true) {
        bool used = false;
        for (int i = 0; i < num_clients; i++) {
            if(Clients[i].id == nextId) {
                used = true;
                break;
            }
        }

        if(!used) return nextId;
        nextId++;
    }
}

void regularMessageAction(int index){
    struct p_msgs newMsg;
    strcpy(newMsg.m, bufferRec);
    newMsg.t = serverTime;
    Clients[index].pastMessages.push_back(newMsg);

    if (Clients[index].pastMessages.size() > 20){
        for(int i = 1; i < Clients[index].pastMessages.size(); i++){
            Clients[index].pastMessages[i-1] = Clients[index].pastMessages[i];
        }
        Clients[index].pastMessages.pop_back();
    }
}

void sendData(socklen_t client_len, const char* data, size_t length){
    //get request and handle it

    size_t lengthOfData;
    for (int i = 0; i < num_clients; i++){

        unsigned char flag  = data[0]; // first byte
        
        uint16_t msgId = static_cast<unsigned char>(data[1])
            | (static_cast<unsigned char>(data[2]) << 8); // little-endian

        switch(flag){
            case 0:
                lengthOfData = 15;
                break;
            case 1:
                lengthOfData = 3;
                break;
            case 2:
                lengthOfData = 3;
                break;
            case 3:
                //this will never happen yet, we get hits not send them, yet
                lengthOfData = 23;
                break;
            case 4:
                //tell client to send their data
                //so: flag | id | pos | time
                lengthOfData = 23;
            default:
                lengthOfData = 15;
                break;
        }

        if (flag == 1 && msgId != Clients[i].id) {
            continue;
        }

        sendto(sock, data, lengthOfData, 0, (sockaddr*)&Clients[i].addr, client_len);

        mvprintw(6+(i*2), 0, to_string(Clients[i].id).c_str());
        mvprintw(6+(i*2), 128, to_string(Clients[i].timeout).c_str());

    }
}

void killUser(int index){
    coutMessage = " removing user";

    Clients[index] = Clients[num_clients - 1];
    Clients.pop_back();
    num_clients--;
}

void recieveData(sockaddr_in* client_addr, socklen_t *client_len){
    while(1){
        int bytes = recvfrom(sock, bufferRec, sizeof(bufferRec), MSG_DONTWAIT,  (sockaddr*)client_addr, client_len);
        if (bytes < 0){
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                break; // no more packets
            perror("recvfrom failed");
            break;
        }


        bool found = false;
        int index = 0;
        lock_guard<mutex> lock(clientMutex);
        for (int i = 0; i < num_clients; i++){
            if (Clients[i].addr.sin_addr.s_addr == client_addr->sin_addr.s_addr && Clients[i].addr.sin_port == client_addr->sin_port){
                found = true;
                index = i;
                break;
            }
        }

        if (!found){
            sockaddr_in new_client;
            Client newClient;
            newClient.addr = *client_addr;
            newClient.timeout = 0;
            uint16_t newID = generateUniqueId();
            newClient.id = newID;
            Clients.push_back(newClient);
            num_clients++;

            coutMessage = "new client";
            index = num_clients - 1;
        }

        Clients[index].timeout = 0;

        std::stringstream ss;
        for (int i = 0; i < 15; i++) {
            ss << "0x" 
                << std::setw(2) << std::setfill('0') 
                << std::hex << static_cast<unsigned int>(static_cast<unsigned char>(bufferRec[i])) 
                << " ";
        }

        //1=new user; 2=kill; 0=regular; 3=hits

        char flag = bufferRec[0];
        switch(flag){
            case 0:
                regularMessageAction(index);
                break;
            case 1:
                if(!found){
                    char buffer[32] = {0};
                    uint16_t id = Clients[index].id;

                    buffer[0] = 1;                        // flag
                    buffer[1] = static_cast<char>(id & 0xFF);        // low byte
                    buffer[2] = static_cast<char>((id >> 8) & 0xFF); // high bytes
                    memcpy(bufferRec, buffer, 3);
                }

                break;
            case 2:
                killUser(index);
                break;
            case 3:
                //this will never happen yet, we get hits not send them, yet
                regularMessageAction(index);
                break;
            case 4:
                regularMessageAction(index);
                //we got the clients timer back;
            default:
                regularMessageAction(index);
                break;
        }

        //getting the offset:
        //if we juust send a clients time itll be offset + latancy, bad
        //so we could send a serverTime first, wait for the clients time, theeen compare the two
        //the client should have gotten the server time in the middle of the start and end
        //the server should then have two server timestamps and one client time, the client send their time, in between the two server times, so the offset is as follows:
        //(serverTime1 + serverTime2) / 2 - clientTime;
        //ill send it with a 4 flag, and get with 4. itll happen every 5-10 seconds
        //so get a start time + endtime leave start, add to end untill elasped > 5-10, then set start=end

        sendData(*client_len, bufferRec, 15);

    }
}
