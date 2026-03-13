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

struct Client{
    sockaddr_in addr;
    int timeout;
    string lastMessage;
    uint16_t id;
    uint16_t ping;
} typedef Client ;

vector<Client> Clients;

string coutMessage;

char bufferRec[32];

uint16_t generateUniqueId() {
    static uint16_t nextId = 1;

    while(true) {
        bool used = false;
        for (int i = 0; i < Clients.size(); i++) {
            if(Clients[i].id == nextId) {
                used = true;
                break;
            }
        }

        if(!used) return nextId;
        nextId++;
    }
}

void sendData(socklen_t client_len, const char* data, size_t length){
    //get request and handle it

    for (int i = 0; i < Clients.size(); i++){

        unsigned char flag  = data[0]; // first byte
        uint16_t msgId = static_cast<unsigned char>(data[1])
            | (static_cast<unsigned char>(data[2]) << 8); // little-endian

        // skip sending to the client that owns this ID if it's a new-user flag
        if (flag == 1 && msgId != Clients[i].id) {
            continue; // do not send this packet back to the same client
        }

        sendto(sock, data, length, 0, (sockaddr*)&Clients[i].addr, client_len);

        mvprintw(6+(i*2), 0, to_string(Clients[i].id).c_str());
        mvprintw(6+(i*2), 16, Clients[i].lastMessage.c_str());
        mvprintw(6+(i*2), 128, to_string(Clients[i].timeout).c_str());

    }
}

void killUser(int index){
    coutMessage = " removing user";

    Clients[index] = Clients[Clients.size() - 1];
    Clients.pop_back();
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
        for (int i = 0; i < Clients.size(); i++){
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
            newClient.lastMessage = "";
            uint16_t newID = generateUniqueId();
            newClient.id = newID;
            Clients.push_back(newClient);

            coutMessage = "new client";
            index = Clients.size() - 1;
        }

        Clients[index].timeout = 0;

        std::stringstream ss;
        for (int i = 0; i < 15; i++) {
            ss << "0x" 
                << std::setw(2) << std::setfill('0') 
                << std::hex << static_cast<unsigned int>(static_cast<unsigned char>(bufferRec[i])) 
                << " ";
        }

        Clients[index].lastMessage = ss.str();

        if(bufferRec[0] == 1){
            // new user
            if(!found){
                char buffer[32] = {0};
                uint16_t id = Clients[index].id;

                buffer[0] = 1;                        // flag
                buffer[1] = static_cast<char>(id & 0xFF);        // low byte
                buffer[2] = static_cast<char>((id >> 8) & 0xFF); // high bytes
                memcpy(bufferRec, buffer, 3);
            }
        }
        else if (bufferRec[0] == 2){
            killUser(index);
        }

        sendData(*client_len, bufferRec, 15);

    }
}
