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
WINDOW* logWin;
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
    uint64_t sendTime;
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

void sendData(socklen_t client_len, const char* data, size_t length)
{
    size_t lengthOfData;

    for (int i = 0; i < Clients.size(); i++)
    {
        unsigned char flag = data[0];

        uint16_t msgId =
            static_cast<unsigned char>(data[1]) |
            (static_cast<unsigned char>(data[2]) << 8);

        switch(flag)
        {
            case 0: lengthOfData = 15; break;
            case 1: lengthOfData = 3;  break;
            case 2: lengthOfData = 3;  break;
            case 3: lengthOfData = 23; break;

            case 4:
                Clients[i].sendTime = serverTime;
                lengthOfData = 3;
                break;

            default:
                lengthOfData = 15;
                break;
        }

        if (flag == 1 && msgId != Clients[i].id)
            continue;

        sendto(sock, data, lengthOfData, 0,
               (sockaddr*)&Clients[i].addr, client_len);

        // ---- client info section ----
        mvprintw(6 + (i*2), 0,  "ID: %d", Clients[i].id);
        mvprintw(6 + (i*2), 20, "Offset: %llu", Clients[i].server_offset);
        mvprintw(6 + (i*2), 40, "Timeout: %d", Clients[i].timeout);

        // ---- packet log ----
        wprintw(logWin, "Send -> Client %d | flag=%d | bytes=%zu\n",
                Clients[i].id, flag, lengthOfData);

        wprintw(logWin, "Data: ");
        for(int j = 0; j < lengthOfData; j++)
            wprintw(logWin, "%02X ", (unsigned char)data[j]);

        wprintw(logWin, "\n\n");
    }

    refresh();
    wrefresh(logWin);
}
void killUser(int index){
    coutMessage = " removing user";

    Clients[index] = Clients[num_clients - 1];
    Clients.pop_back();
    num_clients--;
}

void recieveData(sockaddr_in* client_addr, socklen_t *client_len, bool getOffsets){
    while(1){
        int bytes = recvfrom(sock, bufferRec, sizeof(bufferRec), MSG_DONTWAIT,  (sockaddr*)client_addr, client_len);
        if (bytes < 0){
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                break; // no more packets
            perror("recvfrom failed");
            break;
        }

        char flag = bufferRec[0];
        uint16_t index = static_cast<unsigned char>(bufferRec[1])
            | (static_cast<unsigned char>(bufferRec[2]) << 8); // little-endian
        
        lock_guard<mutex> lock(clientMutex);
        if (flag == 1){
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
        }else{
            //change this once we make it a hash map
            for(int i = 0; i < num_clients; i++){
                if (index == Clients[i].id){
                    //found them
                    index = i;
                }
            }
        }
        Clients[index].timeout = 0;

        std::stringstream ss;
        for (int i = 0; i < 15; i++) {
            ss << "0x" 
                << std::setw(2) << std::setfill('0') 
                << std::hex << static_cast<unsigned int>(static_cast<unsigned char>(bufferRec[i])) 
                << " ";
        }

        char flag_backup = flag;
        if (getOffsets){
            flag = 4;
        }
        //check to see if we need offset update
        //if yes then set the buffer, sendData, and CONTINUE, dont wanna send twice
        

        //1=new user; 2=kill; 0=regular; 3=hits
        switch(flag){
            case 0:
                regularMessageAction(index);
                break;

            case 1:
                {
                    char buffer[32] = {0};
                    uint16_t id = Clients[index].id;

                    buffer[0] = 1;
                    buffer[1] = static_cast<char>(id & 0xFF);
                    buffer[2] = static_cast<char>((id >> 8) & 0xFF);

                    memcpy(bufferRec, buffer, 3);
                    break;
                }

            case 2:
                killUser(index);
                break;

            case 3:
                regularMessageAction(index);
                break;

            case 4:
                {
                    uint16_t id = Clients[index].id;
                    bufferRec[0] = flag;
                    bufferRec[1] = static_cast<char>(id & 0xFF);
                    bufferRec[2] = static_cast<char>((id >> 8) & 0xFF);


                    if (flag_backup == 0) regularMessageAction(index);
                    break;
                }
            case 5:
                {
                    uint16_t id = Clients[index].id;
                    bufferRec[0] = flag;
                    bufferRec[1] = static_cast<char>(id & 0xFF);
                    bufferRec[2] = static_cast<char>((id >> 8) & 0xFF);
                    uint64_t clientTime =
                        (uint64_t)(unsigned char)bufferRec[3]
                        | ((uint64_t)(unsigned char)bufferRec[4] << 8)
                        | ((uint64_t)(unsigned char)bufferRec[5] << 16)
                        | ((uint64_t)(unsigned char)bufferRec[6] << 24)
                        | ((uint64_t)(unsigned char)bufferRec[7] << 32)
                        | ((uint64_t)(unsigned char)bufferRec[8] << 40)
                        | ((uint64_t)(unsigned char)bufferRec[9] << 48)
                        | ((uint64_t)(unsigned char)bufferRec[10] << 56);

                    coutMessage = "got offset ";
                    Clients[index].server_offset = (serverTime + Clients[index].sendTime) / 2 - clientTime;
                    break;

                }


            default:
                regularMessageAction(index);
                break;
        }

        sendData(*client_len, bufferRec, 15);

    }
}
