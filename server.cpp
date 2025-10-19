#include <iostream>
#include <sys/socket.h>
#include <thread>
#include <vector>
#include <unistd.h>
#include <netinet/in.h>
#include <map>
#include <sys/select.h>
#include <atomic>
#include <string>
#include "headers/client.hpp"
#include <cstring>
#include <poll.h>
#include <mutex>
#include "headers/utilities.hpp"

using namespace std;
atomic<bool> serverRunning(true);
map<int, Client> clients;
map<int, thread> cliThreads;
map<int, int> cliSocks;
map<int, thread> exiHandlers;

mutex clientsMutex;

#define maxClients 100
#define maxMessageLen 1000

void cleanupClient(int cliNum)
{
    {
        lock_guard<mutex> lock(clientsMutex);
        clients.erase(cliNum);
        cliSocks.erase(cliNum);
    }
}

void broadCastDataToAll()
{
    {
    lock_guard<mutex> lock(clientsMutex);

    if (clients.empty())
        return;

    int numClients = clients.size();
    size_t si = 3 + numClients * Client::serializedSize();

    char buffer[si];

    std::memcpy(buffer, "CLI", 3);

    size_t offset = 3;
    for (auto& client : clients) {
        client.second.serialize(buffer + offset);
        offset += client.second.serializedSize();
    }

    size_t sizeBuffSize = 7;
    char *sizeBuffer = new char[sizeBuffSize];
    std::memcpy(sizeBuffer, "LIS", 3);
    std::memcpy(sizeBuffer + 3, &numClients, sizeof(numClients));

    for (auto sock : cliSocks)
    {
        if (send(sock.second, sizeBuffer, sizeBuffSize, 0) < 0)
        {
            cerr << "Error in Sending Size of Buffer\n";
            continue;
        }
    }

    for(auto sock : cliSocks){
        if(send(sock.second,buffer,si,0) < 0){
            cerr << "Error in Sending clients data\n";
            continue;
        }
    }

    delete[] sizeBuffer;
}
}

// void handleClient(int cliSock, int cliNum)
// {
//     if (serverRunning)
//     {
//         if (cliSock < 0) return;
//             char s[51] = {0};
//             int bytesReceived = recv(cliSock, s, sizeof(s) - 1, 0);

//             if (bytesReceived > 0)
//             {
//                 s[bytesReceived] = '\0'; // Null-terminate the received data
//                 string p = s;

//                 Client newClient = Client(cliNum, s);

//                 {
//                     lock_guard<mutex> lock(clientsMutex);
//                     clients[cliNum] = newClient;
//                 }

//                 size_t buffSize = newClient.serializedSize();
//                 char buffer[buffSize];
//                 newClient.serialize(buffer);

//                 send(cliSock, buffer, buffSize, 0);

//                 {
//                     lock_guard<mutex> lock(clientsMutex);
//                     cliSocks[cliNum] = cliSock;
//                 }

//                 broadCastDataToAll();
//             }
//             else if (bytesReceived == 0)
//             {
//                 // Client disconnected
//                 cout << "Client " << cliNum << " disconnected." << endl;
//                 close(cliSock);

//                 cleanupClient(cliNum);

//                 return;
//             }
//             else
//             {
//                 // recv() error
//                 perror("recv() failed");
//                 close(cliSock);

//                 cleanupClient(cliNum);

//                 return;
//             }
//     }

//     while(serverRunning){
//         fd_set readFd;
//         FD_ZERO(&readFd);
//         FD_SET(cliSock, &readFd);

//         struct timeval tv;
//         tv.tv_sec = 0;
//         tv.tv_usec = 500;

//         int activity = select(maxClients + 1,&readFd,NULL,NULL,&tv);

//         if(activity > 0){
//             char buff[maxMessageLen+1];

//             if(recv(cliSock,buff,maxMessageLen,0) > 0){
//                 buff[maxMessageLen] = '\0';
                
//                 if(strncmp(buff,"MSG",3)){
//                     int toSend = buff[3];
//                     {
//                         lock_guard<mutex> lock(clientsMutex);

//                         buff[3] = clients[cliNum].id;

//                         if(cliSocks.find(toSend) != cliSocks.end()){
//                             int sendSock = cliSocks[toSend];

//                             int sentBytes = send(sendSock,buff,maxMessageLen,0);

//                             if(sentBytes == maxMessageLen){
//                                 cout<<"MESSAGE SENT TO "<<toSend<<" Successfully\n";
//                                 continue;
//                             }

//                             else if(sentBytes == 0){
//                                 cerr<<"Problem Sending Message:";
//                                 break;
//                             }

//                             else if(sentBytes < 0){
//                                 cerr<<"Critical error, line -> "<<__LINE__<<endl;
//                                 break;
//                             }

//                         }

//                     }
//                 }
//             }
//         }
//     }
//     cleanupClient(cliNum);
// }

// void exitHandler(int sock)
// {
//     while (serverRunning)
//     {
//         if (sock < 0)
//             break;

//         fd_set readFd;
//         FD_ZERO(&readFd);
//         FD_SET(sock, &readFd);

//         struct timeval tv;
//         tv.tv_sec = 1;
//         tv.tv_usec = 0;

//         int activity = select(maxClients + 1, &readFd, NULL, NULL, &tv);

//         if (activity > 0 && FD_ISSET(sock, &readFd))
//         {
//             char buff[7];

//             recv(sock, buff, 7, 0);

//             if (strncmp(buff, "EXI", 3) == 0) // returns zero if true
//             {
//                 string s = buff;

//                 int exited = static_cast<unsigned char>(buff[3]); ;

//                 close(sock);

//                 cleanupClient(exited);

//                 cout << "Client " << exited << " Disconnected\n";

//                 break;
//             }
//         }
//     }
// }

void handleClient(int cliSock, int cliNum) {
    if (!serverRunning || cliSock < 0) return;

    char s[51] = {0};
    int bytesReceived = recv(cliSock, s, sizeof(s) - 1, 0);
    if (bytesReceived <= 0) {
        if (bytesReceived == 0) {
            cout << "Client " << cliNum << " disconnected during handshake.\n";
        } else {
            perror("recv() failed during handshake");
        }
        close(cliSock);
        cleanupClient(cliNum);
        return;
    }

    s[bytesReceived] = '\0';
    Client newClient(cliNum, s);
    {
        lock_guard<mutex> lock(clientsMutex);
        clients[cliNum] = newClient;
        cliSocks[cliNum] = cliSock;
    }

    size_t buffSize = newClient.serializedSize();
    char buffer[buffSize];
    newClient.serialize(buffer);
    send(cliSock, buffer, buffSize, 0);

    broadCastDataToAll();

    while (serverRunning) {
        fd_set readFd;
        FD_ZERO(&readFd);
        FD_SET(cliSock, &readFd);

        struct timeval tv;
        tv.tv_sec = 0;  
        tv.tv_usec = 500;

        int activity = select(cliSock + 1, &readFd, NULL, NULL, &tv);

        if (activity < 0) {
            perror("select() error");
            break;
        }

        if (activity == 0) {
            continue;
        }

        if (FD_ISSET(cliSock, &readFd)) {
            char buff[maxMessageLen + 1] = {0};
            bytesReceived = recv(cliSock, buff, maxMessageLen, 0);

            if (bytesReceived <= 0) {
                if (bytesReceived == 0) {
                    cout << "Client " << cliNum << " disconnected.\n";
                } else {
                    perror("recv() failed");
                }
                break;
            }

            if (strncmp(buff, "EXI", 3) == 0) {
                cout << "Client " << cliNum << " requested disconnect.\n";
                break;
            }

            else if (strncmp(buff, "MSG", 3) == 0) {
                int toSend = getId(buff);
                {
                    int fromId = clients[cliNum].id;
                    lock_guard<mutex> lock(clientsMutex);
                    fixBuffer(buff,fromId);

                    if (cliSocks.find(toSend) != cliSocks.end()) {
                        int sendSock = cliSocks[toSend];
                        int sentBytes = send(sendSock, buff, maxMessageLen, 0);

                        if (sentBytes == maxMessageLen) {
                            cout << "MESSAGE SENT TO " << toSend << " successfully\n";
                        } else if (sentBytes <= 0) {
                            cerr << "Problem sending message to " << toSend << "\n";
                        }
                    }
                }
            }
        }
    }

    close(cliSock);
    cleanupClient(cliNum);
}

int main()
{
    // int sock = socket(AF_INET, SOCK_STREAM, 0);
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in servAddr;
    servAddr.sin_port = htons(8080);
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        cerr << "Error in Setting socket option ";
        return -1;
    }

    if (bind(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    {
        cerr << "ERROR IN BINDING ";
        return -1;
    }

    if (listen(sock, 10) < 0)
    {
        cerr << "ERROR IN LISTENING ";
        return -1;
    }

    thread adminThread([]()
                       {
        cout<<"ADMIN THREAD RUNNING\n";
        string s;
        while(true){
            cin>>s;
            if(s=="exit"){
                serverRunning = false;
                break;
            }
        } });

    int cliNum = 0;
    while (serverRunning)
    {
        sockaddr_in cliAddr;
        socklen_t cliLen = sizeof(cliAddr);

        struct pollfd fds[1];
        fds[0].fd = sock;
        fds[0].events = POLLIN; // Monitor for incoming data

        int activity = poll(fds, 1, 1000); // 1000 ms timeout
        if (activity > 0 && (fds[0].revents & POLLIN))
        {
            // There is an incoming connection, call accept
            int cliSock = accept(sock, (struct sockaddr *)&cliAddr, &cliLen);
            if (cliSock < 0)
            {
                if (errno == EWOULDBLOCK || errno == EAGAIN)
                {
                    // No connection available, continue
                    continue;
                }
                else
                {
                    cerr << "Accept error.\n";
                    continue;
                }
            }

            // Handle the new client connection
            cliThreads[cliNum] = thread(handleClient, cliSock, cliNum);
            // exiHandlers[cliNum] = thread(exitHandler,cliSock);
            cliNum++;
        }
        else if (activity == 0)
        {
            // Timeout occurred, no incoming connections
            continue;
        }
        else
        {
            // Error in poll
            cerr << "Poll error.\n";
            continue;
        }
    }

    for (auto &i : cliThreads)
    {
        if(i.second.joinable()){
            i.second.join();
        }
    }

    for (auto &i : exiHandlers)
    {
        if(i.second.joinable()){
            i.second.join();
        }
    }

    adminThread.join();

    for (auto &i : cliSocks)
    {
        close(i.second);
    }

    cout << "Closing Server\n";
    close(sock);
    return 0;
}