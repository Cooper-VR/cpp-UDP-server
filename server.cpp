#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

int main(){

    //first we make a socket to put everything through
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    char ip [INET_ADDRSTRLEN] = "";

    //we need to make a struct that is binded to the socket
    struct sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(7777);
    address.sin_addr.s_addr = INADDR_ANY;

    //this is so that we can reuse the socket and not have to wait a min after useage
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    //we then bind the socket to the struct
    int bindRes = bind(sock, (sockaddr*)&address, sizeof(address));
    if (bindRes < 0){
        perror("bind failed");
        return 1;
    }

    //we listen o=n that socket, only 1 person can be there at a time
    listen(sock, 1);

    vector<sockaddr_in> client_addrs;

    while (1){

        //we make a client socket
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        char bufferRec[1024] = { 0 };
        int bytes = recvfrom(sock, bufferRec, sizeof(bufferRec), 0,  (sockaddr*)&client_addr, &client_len);
        if (bytes < 0){
            perror("recvfrom failed");
            break;
        }



        bool found = false;
        int index = 0;;
        for (int i = 0; i < client_addrs.size(); i++){
            if (client_addrs[i].sin_addr.s_addr == client_addr.sin_addr.s_addr && client_addrs[i].sin_port == client_addr.sin_port){
                found = true;
                index = i;
                break;
            }
        }

        if (!found){
            sockaddr_in new_client;
            new_client = client_addr;
            client_addrs.push_back(new_client);
            cout << "new client" << endl;
            index = client_addrs.size() - 1;
        }

        if (strncmp(bufferRec, "killUser", bytes) == 0){
            cout << "removing user" << endl;
            sockaddr_in tempClient = client_addrs[index];
            client_addrs[index] = client_addrs[client_addrs.size() - 1];
            client_addrs.pop_back();
        }
        //get request and handle it
        char buffer[1024] = { 0 };
        strcpy(buffer, "hello from the server");

        for (int i = 0; i < client_addrs.size(); i++){
            sendto(sock, buffer, sizeof(buffer), 0, (sockaddr*)&client_addrs[i], client_len);
        }

        //send it, kinda strange, not nessesary but will work
    }
    close(sock);

    return 0;
}
