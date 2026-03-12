#include <iostream>
#include <cstring>
#include <ratio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include "serverHelper.h"

using namespace std;

const int TICK_MS = 16;

void threadedTimer(){
    cout << "this is threaded\n";
    auto start = chrono::steady_clock::now();
    auto end = chrono::steady_clock::now();

    while(1){

        start = chrono::steady_clock::now();

        auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);


        lock_guard<mutex> lock(clientMutex);
        for (int i = 0; i < clientTimeouts.size(); i++){
            clientTimeouts[i] += elapsed.count();
            if (clientTimeouts[i] > 10000){
                killUser(i);
            }
        }

        auto end = chrono::steady_clock::now();

    }
}

int main(){
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

    thread t(threadedTimer);

    //detach makes it run at the same time
    t.detach();

    while (1){

        //we make a client socket
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        auto start = chrono::steady_clock::now();

        cout << "doing networking" << endl;

        recieveData(&client_addr, &client_len);
        sendData(client_len);

        auto end = chrono::steady_clock::now();

        auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);

        if (elapsed.count() < TICK_MS){
            this_thread::sleep_for(chrono::milliseconds(TICK_MS - elapsed.count()));
        }
        
    }
    close(sock);

    return 0;
}
