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

vector<sockaddr_in> client_addrs;
vector<int> clientTimeouts;
vector<string> lastMessages;
vector<uint16_t> client_id;

string coutMessage;

char bufferRec[32];

uint16_t generateUniqueId() {
    static uint16_t nextId = 1;

    while(true) {
        bool used = false;
        for (int i = 0; i < client_id.size(); i++) {
            if(client_id[i] == nextId) {
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

    for (int i = 0; i < client_addrs.size(); i++){

        unsigned char flag  = data[0]; // first byte
        uint16_t msgId = static_cast<unsigned char>(data[1])
            | (static_cast<unsigned char>(data[2]) << 8); // little-endian

        // skip sending to the client that owns this ID if it's a new-user flag
        if (flag == 1 && msgId != client_id[i]) {
            continue; // do not send this packet back to the same client
        }

        sendto(sock, data, length, 0, (sockaddr*)&client_addrs[i], client_len);

        mvprintw(6+(i*2), 0, to_string(client_id[i]).c_str());
        mvprintw(6+(i*2), 16, lastMessages[i].c_str());
        mvprintw(6+(i*2), 128, to_string(clientTimeouts[i]).c_str());

    }
}

void killUser(int index){
    coutMessage = " removing user";
    sockaddr_in tempClient = client_addrs[index];
    int tempTime = clientTimeouts[index];
    string tempMsg = lastMessages[index];

    client_addrs[index] = client_addrs[client_addrs.size() - 1];
    clientTimeouts[index] = clientTimeouts[clientTimeouts.size() - 1];
    lastMessages[index] = lastMessages[lastMessages.size() - 1];
    client_id[index] = client_id[client_id.size() - 1];
    clientTimeouts.pop_back();
    client_addrs.pop_back();
    lastMessages.pop_back();
    client_id.pop_back();

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
        for (int i = 0; i < client_addrs.size(); i++){
            if (client_addrs[i].sin_addr.s_addr == client_addr->sin_addr.s_addr && client_addrs[i].sin_port == client_addr->sin_port){
                found = true;
                index = i;
                break;
            }
        }

        if (!found){
            sockaddr_in new_client;
            new_client = *client_addr;
            client_addrs.push_back(new_client);
            clientTimeouts.push_back(0);
            lastMessages.push_back("");
            uint16_t newID = generateUniqueId();
            client_id.push_back(newID);
            coutMessage = "new client";
            index = client_addrs.size() - 1;
        }

        clientTimeouts[index] = 0;

        std::stringstream ss;
        for (int i = 0; i < 15; i++) {
            ss << "0x" 
                << std::setw(2) << std::setfill('0') 
                << std::hex << static_cast<unsigned int>(static_cast<unsigned char>(bufferRec[i])) 
                << " ";
        }

        lastMessages[index] = ss.str();

        if(bufferRec[0] == 1){
            // new user
            if(!found){
                char buffer[32] = {0};
                uint16_t id = client_id[index];

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
