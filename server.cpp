#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include "serverHelper.h"
#include <ncurses.h>
#include <fcntl.h>

using namespace std;

const int TICK_MS = 16;

auto serverStart = chrono::steady_clock::now();
auto offsetStart = chrono::steady_clock::now();
auto offsetEnd = chrono::steady_clock::now();
auto offsetElasped = chrono::duration_cast<chrono::milliseconds>(offsetEnd - offsetStart);


void threadedTimer(){
    auto start = chrono::steady_clock::now();
    auto end = chrono::steady_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);

    while(1){

        start = chrono::steady_clock::now();
        this_thread::sleep_for(chrono::milliseconds(100));

        lock_guard<mutex> lock(clientMutex);
        for (int i = num_clients-1; i >= 0; i--){
            Clients[i].timeout += elapsed.count();
            if (Clients[i].timeout > 10000){
                coutMessage = "killing user -> " + Clients[i].timeout;
                killUser(i);
            }
        }

        auto end = chrono::steady_clock::now();

        elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);

    }
}


void initUI()
{
    initscr();
    cbreak();
    noecho();
    curs_set(0);

    int h, w;
    getmaxyx(stdscr, h, w);

    // bottom half of screen for logs
    logWin = newwin(h/2, w, h/2, 0);
    scrollok(logWin, TRUE);
}

int main(){
    initUI();
    address.sin_family = AF_INET;
    address.sin_port = htons(7777);
    address.sin_addr.s_addr = INADDR_ANY;

    //this is so that we can reuse the socket and not have to wait a min after useage
    int opt = 1;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    fcntl(sock, F_SETFL, O_NONBLOCK);


    //we then bind the socket to the struct
    int bindRes = bind(sock, (sockaddr*)&address, sizeof(address));
    if (bindRes < 0){
        perror("bind failed");
        return 1;
    }

    thread t(threadedTimer);

    //detach makes it run at the same time
    t.detach();
    offsetStart = chrono::steady_clock::now();
    offsetEnd = chrono::steady_clock::now();
    while (1){
        clear();

        mvprintw(0, 0, "Server Running");
        mvprintw(4, 0, "Timeouts: 10000");
        mvprintw(2, 0, "Clients: %d", Clients.size());
        mvprintw(2, 0, "Time: %d", serverTime);

        //we make a client socket
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        auto start = chrono::steady_clock::now();
        serverTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - serverStart
                    ).count();

        auto offsetElasped = chrono::duration_cast<chrono::milliseconds>(offsetEnd - offsetStart);
        if (offsetElasped.count() > 10000){
            recieveData(&client_addr, &client_len, true);
            offsetStart = chrono::steady_clock::now();
            coutMessage = "getting offsets";
        }else{
            recieveData(&client_addr, &client_len, false);
        }
        mvprintw(0, 32, coutMessage.c_str());

        refresh();

        //napms(100);
        auto end = chrono::steady_clock::now();

        auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start);

        if (elapsed.count() < TICK_MS){
            this_thread::sleep_for(chrono::milliseconds(TICK_MS - elapsed.count()));
        }

        offsetEnd = chrono::steady_clock::now();
        
    }

    endwin();

    close(sock);
    

    return 0;
}
