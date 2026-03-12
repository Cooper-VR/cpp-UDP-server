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

using namespace std;

vector<int> clientTimeouts;
mutex clientMutex;

int sock = socket(AF_INET, SOCK_DGRAM, 0);
char ip [INET_ADDRSTRLEN] = "";
struct sockaddr_in address{};

vector<sockaddr_in> client_addrs;

void killUser(int index){
    cout << "removing user" << endl;
    sockaddr_in tempClient = client_addrs[index];
    int tempTime = clientTimeouts[index];

    client_addrs[index] = client_addrs[client_addrs.size() - 1];
    clientTimeouts[index] = clientTimeouts[clientTimeouts.size() - 1];
    clientTimeouts.pop_back();
    client_addrs.pop_back();

}

void recieveData(sockaddr_in* client_addr, socklen_t *client_len){
    char bufferRec[1024] = { 0 };
    int bytes = recvfrom(sock, bufferRec, sizeof(bufferRec), 0,  (sockaddr*)client_addr, client_len);
    if (bytes < 0){
        perror("recvfrom failed");
        return;
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
        cout << "new client" << endl;
        index = client_addrs.size() - 1;
    }

    clientTimeouts[index] = 0;

    if (strncmp(bufferRec, "killUser", bytes) == 0){
        killUser(index);
    }
}

void sendData(socklen_t client_len){
    //get request and handle it
    char buffer[1024] = { 0 };
    strcpy(buffer, "hello from the server");

    lock_guard<mutex> lock(clientMutex);

    for (int i = 0; i < client_addrs.size(); i++){
        sendto(sock, buffer, strlen(buffer), 0, (sockaddr*)&client_addrs[i], client_len);
    }
}
