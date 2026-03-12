#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <time.h>

using namespace std;

vector<sockaddr_in> client_addrs;
vector<double> clientTimes;

void killUser(int index){
    cout << "removing user" << endl;
    sockaddr_in tempClient = client_addrs[index];
    double time = clientTimes[index];
    clientTimes[clientTimes.size() - 1] = clientTimes[index];
    client_addrs[index] = client_addrs[client_addrs.size() - 1];
    client_addrs.pop_back();
    clientTimes.pop_back();

}

int main(){

    time_t prev = time(0);
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
        int index = 0;
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
            clientTimes.push_back(0.0); 
            index = client_addrs.size() - 1;
            clientTimes[index] = 0;
        }

        if (strncmp(bufferRec, "killUser", bytes) == 0){
            killUser(index);
        }
        //get request and handle it
        char buffer[1024] = { 0 };
        strcpy(buffer, "hello from the server");

        for (int i = 0; i < client_addrs.size(); i++){
            clientTimes[i] += difftime(time(0), prev);
            sendto(sock, buffer, sizeof(buffer), 0, (sockaddr*)&client_addrs[i], client_len);
        }

        prev = time(0);

        //send it, kinda strange, not nessesary but will work
    }
    close(sock);

    return 0;
}
