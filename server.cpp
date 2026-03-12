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

void handleRequest(int clientSocket, char* buffer){

    //get if post or get
    string type;
    for (int i = 0; i < 4; i++){
        type += buffer[i];
    }

    //get url data we need
    string path;
    string fileExtention;
    bool foundDot = false;
    for (int i = 5; i < 1024; i++){
        
        if (foundDot) fileExtention += buffer[i];
        if (buffer[i] == '.')
            foundDot = true;

        if (buffer[i] == ' '){
            break;
        }
        path += buffer[i];
    }

    cout << "Path: " << path << endl;
    cout << "File extention:" << fileExtention << endl;
    if (type == "GET "){
        //get request

        if (path == ""){
            path = "index.html";
            fileExtention = "html";
        }

        
        //read requested file
        ifstream f(path);
        string s;
        string b;
        while(getline(f, s))
            b += s + '\n';
        f.close();


        //construct message
        ostringstream message;
        message << "HTTP/1.1 200 OK\r\n";
        if (fileExtention == "html")
            message << "Content-Type: text/html\r\n";
        else if (fileExtention == "css")
            message << "Content-Type: text/css\r\n";
        else if (fileExtention == "js")
            message << "Content-Type: text/javascript\r\n";
        else if (fileExtention == "mp4")
            message << "Content-Type: video/mp4\r\n";
        else if (fileExtention == "mpeg")
            message << "Content-Type: video/mpeg\r\n";
        else if (fileExtention == "png")
            message << "Content-Type: image/png\r\n";
        else if (fileExtention == "jpeg")
            message << "Content-Type: image/jpeg\r\n";
        else if (fileExtention == "jpg")
            message << "Content-Type: image/jpg\r\n";
        else
            message << "Content-Type: text/plain\r\n";



        message << "Content-Length:" << b.size() << "\r\n";
        message << "Connection: close\r\n";
        message << "\r\n";
        message << b;

        string response = message.str();

        //send it, kinda strange, not nessesary but will work
        size_t total = 0;
        while(total < response.size()){
            ssize_t sent = send(clientSocket, response.c_str()+total, response.size()-total, 0);
            if (sent <= 0) break;
            total += sent;
        }



    }else if (type == "POST"){
        //post request
        
        //not implemented
    }

    
}

int main(){

    //first we make a socket to put everything through
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    char ip [INET_ADDRSTRLEN] = "";

    //we need to make a struct that is binded to the socket
    struct sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
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

    //we listen on that socket, only 1 person can be there at a time
    listen(sock, 1);

    while (1){
        //we make a client socket
        int clientSocket = accept(sock, nullptr, nullptr);

        //we fork here, one for listening to new people, another for handling clients
        pid_t id1 = fork();

        //made a new fork
        if (id1 == 0){
            close(sock);

            //get request and handle it
            char buffer[1024] = { 0 };
            recv(clientSocket, buffer, sizeof(buffer), 0);
            cout << buffer << endl;
            handleRequest(clientSocket, buffer);
            
            close(clientSocket);
            exit(0);
        }else{
            close(clientSocket);
        }
    }
    close(sock);

    return 0;
}
