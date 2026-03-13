#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <signal.h>

using namespace std;

int sock;
struct sockaddr_in address{};

void handle_exit(int sig){
    string msg = "killUser";
    sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&address, sizeof(address));
    cout << "killed user" << endl;
    close(sock);
    exit(0);
}

int main(){
    signal(SIGINT, handle_exit);

    //first we make a socket to put everything through
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    char ip [INET_ADDRSTRLEN] = "";

    //we need to make a struct that is binded to the socket
    address.sin_family = AF_INET;
    address.sin_port = htons(7777);
    address.sin_addr.s_addr = inet_addr("127.0.0.1");

    //this is so that we can reuse the socket and not have to wait a min after useage
    
    string msg = "hello from the client";

    while(1){
        sendto(sock, msg.c_str(), sizeof(msg), 0, (sockaddr*)&address, sizeof(address));

        char buffer[1024] = { 0 };
        sockaddr_in from;
        socklen_t from_len = sizeof(from);

        int bytes = recvfrom(sock, buffer, sizeof(buffer), 0, (sockaddr*)&from, &from_len);

        cout << buffer << endl;
    }



    close(sock);

    return 0;
}
