#include <iostream>
#include <string>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "headers/client.hpp"
#include <atomic>
#include <thread>
#include <signal.h>
#include<mutex>
#include "headers/utilities.hpp"

#define sizeBuffSize 7
#define maxClients 100
#define maxMessageLen 1000

using namespace std;
atomic<bool> connected(true);
int sock;
atomic<bool> interrupted(false);
mutex sockMutex;

Client myID;

void handleSending(){
    while(connected && !interrupted){
        string s;
        getline(cin,s);

        if(!connected || interrupted) break;

        if(s == "exit"){
            char exiBuffer[7] = {0};
            std::memcpy(exiBuffer, "EXI", 3);
            std::memcpy(exiBuffer + 3, &myID.id, sizeof(myID.id));

            send(sock, exiBuffer, sizeof(exiBuffer), 0);
            cout << "Exiting\n";
            connected = false;
            break;
        }

        else if(strncmp(s.c_str(),"MSG",3) == 0){
            char *buff = new char[maxMessageLen];

            strcpy(buff,s.c_str());

            {
                lock_guard<mutex> lock(sockMutex);

                if(send(sock,buff,maxMessageLen,0) > 0){
                    // cout<<s<<endl;
                }
                else{
                    cerr<<"ERROR SENDING MESSAGE:";
                    delete[] buff;
                    break;
                }

            }

            delete[] buff;
        }
    }
}

void handleReceiving(int sock)
{
    while (connected && !interrupted)
    {
        fd_set readfd;
        FD_ZERO(&readfd);
        FD_SET(sock, &readfd);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100;

        int activity = select(sock + 1, &readfd, NULL, NULL, &tv);

        if(!connected || interrupted) break;

        if (activity > 0 && FD_ISSET(sock, &readfd))
        {
            char buff[maxMessageLen];
            if (recv(sock, buff, sizeBuffSize, 0) > 0)
            {
                string s(buff, 3);

                if (s == "LIS")
                {

                    int size;
                    std::memcpy(&size, buff + 3, 4);

                    char buffer[3 + size * Client::serializedSize()];

                    if (recv(sock,&buffer,3 * size * Client::serializedSize(),0) > 0){

                        string p(buffer,3);
                        if(p == "CLI"){
                            cout<<"Clients Available on server\n";
                            
                            for(int i = 0;i<size;i++){
                                Client temp;
                                temp.deserialize(buffer + 3 + i*Client::serializedSize());
                                cout<<temp.id<<" -> "<<temp.name<<endl;
                            }
                        }

                    }
                    else{
                        cerr<<"Error receiving clients\n";
                        continue;
                    }
                }
                else if(s == "MSG"){
                    recv(sock,buff+sizeBuffSize,maxMessageLen,0);
                    int from = getId(buff);

                    cout<<"MESSAGE FROM "<<from<<" -> ";
                    if(buff[4] == ' '){
                        for(int i = 5;i<maxMessageLen;i++){
                            if(buff[i] == '\0'){
                                break;
                            }
                            cout<<buff[i];
                        }
                    }
                    else{
                        for(int i = 6;i<maxMessageLen;i++){
                            if(buff[i] == '\0'){
                                break;
                            }
                            cout<<buff[i];
                        }
                    }
                    cout<<endl;
                }

            }
            else
            {
                cerr << "ERROR RECEIVING\n";
                break;
            }
        }
    }
}

// void handleExit(int sock)
// {
//     string s;
//     while (connected && !interrupted)
//     {
//         if(cin >> s){
//             if (s == "exit")
//         {
//             char exiBuffer[7] = {0};
//             std::memcpy(exiBuffer, "EXI", 3);
//             std::memcpy(exiBuffer + 3, &myID.id, sizeof(myID.id));

//             send(sock, exiBuffer, sizeof(exiBuffer), 0);
//             cout << "Exiting\n";
//             connected = false;
//             break;
//         }
//         }
//         else if(interrupted || !connected){
//             break;
//         }
//     }
// }



void SigIntHandler(int s)
{

    interrupted = true;

    char exiBuffer[7] = {0};
    std::memcpy(exiBuffer, "EXI", 3);
    std::memcpy(exiBuffer + 3, &myID.id, sizeof(myID.id));

    send(sock, exiBuffer, sizeof(exiBuffer), 0);
    cout << "Exiting\n";
    connected = false;

}

int main()
{

    signal(SIGINT, SigIntHandler);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in servAddr;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(8080);
    servAddr.sin_family = AF_INET;

    if (connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    {
        cout << "ERROR CONNECTING\n";
        return 0;
    }

    string s;
    cout << "Enter Name to send to server:";
    cin >> s;
    cin.clear();

    send(sock, s.c_str(), s.size(), 0);

    char buffer[Client::serializedSize()];
    if (recv(sock, buffer, sizeof(buffer), 0) > 0)
    {
        myID.deserialize(buffer);

        thread broadCastThread(handleReceiving, sock);
        // thread adminThread(handleExit, sock);
        thread sendingThread(handleSending);

        cout<<"MESSAGE FORMAT IS -> {MSG<id> <message>}\n";

        broadCastThread.join();
        // adminThread.join();
        sendingThread.join();

        close(sock);
    }
    else
    {
        cerr << "ERROR RECEIVING ID FROM SERVER\n";
        close(sock);
    }
}