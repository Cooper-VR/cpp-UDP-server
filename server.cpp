#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>

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

    while (1){

        //we make a client socket
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        //get request and handle it
        char bufferRec[1024] = { 0 };

        int bytes = recvfrom(sock, bufferRec, sizeof(bufferRec), 0,  (sockaddr*)&client_addr, &client_len);
        if (bytes < 0){
            perror("recvfrom failed");
            break;
        }
        cout << bufferRec << endl;

        //get request and handle it
        char buffer[1024] = { 0 };
        strcpy(buffer, "hello from the server");

        sendto(sock, buffer, sizeof(buffer), 0, (sockaddr*)&client_addr, client_len);

        //send it, kinda strange, not nessesary but will work
    }
    close(sock);

    return 0;
}
