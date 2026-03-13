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

using namespace std;

mutex clientMutex;

int sock = socket(AF_INET, SOCK_DGRAM, 0);
char ip [INET_ADDRSTRLEN] = "";
struct sockaddr_in address{};

vector<sockaddr_in> client_addrs;
vector<int> clientTimeouts;
vector<string> lastMessages;

string coutMessage;

void killUser(int index){
    coutMessage += " removing user";
    sockaddr_in tempClient = client_addrs[index];
    int tempTime = clientTimeouts[index];
    string tempMsg = lastMessages[index];

    client_addrs[index] = client_addrs[client_addrs.size() - 1];
    clientTimeouts[index] = clientTimeouts[clientTimeouts.size() - 1];
    lastMessages[index] = lastMessages[lastMessages.size() - 1];
    clientTimeouts.pop_back();
    client_addrs.pop_back();
    lastMessages.pop_back();

}

void recieveData(sockaddr_in* client_addr, socklen_t *client_len){
    while(1){
        char bufferRec[1024] = { 0 };
        int bytes = recvfrom(sock, bufferRec, sizeof(bufferRec), MSG_DONTWAIT,  (sockaddr*)client_addr, client_len);
        if (bytes < 0){
            perror("recvfrom failed");
            break;
        }

        bufferRec[bytes] = '\0';
        string msg(bufferRec);

        coutMessage = "got this" + msg;


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
            coutMessage = "new client";
            index = client_addrs.size() - 1;
        }

        clientTimeouts[index] = 0;
        lastMessages[index] = bufferRec;


        if (msg == "killUser"){
            coutMessage = "got kill message";
            killUser(index);
        }
    }
}

void sendData(socklen_t client_len){
    //get request and handle it
    char buffer[1024] = { 0 };
    strcpy(buffer, "hello from the server");

    lock_guard<mutex> lock(clientMutex);
    mvprintw(2, 0, "Clients: %d", client_addrs.size());
    for (int i = 0; i < client_addrs.size(); i++){
        sendto(sock, buffer, strlen(buffer), 0, (sockaddr*)&client_addrs[i], client_len);
        mvprintw(6+(i*2), 0, lastMessages[i].c_str());
        mvprintw(6+(i*2), 32, to_string(clientTimeouts[i]).c_str());

    }
}
