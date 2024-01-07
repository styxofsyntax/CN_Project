#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
// #include <pthread.h>
#include <thread>

#include <vector>
#include <sstream>
#include <map>

using namespace std;

#define SERVER_PORT = 80;

struct client_ctx
{
    int connfd;
    sockaddr_in c_addr;
};

struct peer_data
{
    string dir, ip;
    int port;
    vector<string> files;
};

void *sendToClient(void *);

class Server
{
private:
    int port, fd;
    map<string, peer_data> peers;

public:
    Server(int port)
    {
        this->port = port;
    }

    void start()
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == -1)
        {
            perror("Socket creation failed : ");
            exit(-1);
        }

        struct sockaddr_in s_addr;
        s_addr.sin_addr.s_addr = INADDR_ANY;
        s_addr.sin_family = AF_INET;
        s_addr.sin_port = htons(80);

        if (bind(fd, (struct sockaddr *)(&s_addr), sizeof(s_addr)) == -1)
        {
            perror("Bind failed on socket : ");
            exit(-1);
        }

        int backlog = 4;
        if (listen(fd, backlog) == -1)
        {
            perror("listen failed on socket : ");
            exit(-1);
        }

        cout << "Server started!\n\n";

        thread acceptThread([this, fd]()
                            { this->invokeAccept(fd); });
        acceptThread.detach();
    }

    void invokeAccept(int fd)
    {
        struct sockaddr_in c_addr;
        socklen_t cliaddr_len = sizeof(c_addr);

        while (1)
        {
            int connfd = accept(fd, (struct sockaddr *)&c_addr, &cliaddr_len);
            if (connfd < 0)
            {
                perror("accept failed on socket : ");
                exit(-1);
            }
            else
            {
                client_ctx c_ctx;
                c_ctx.connfd = connfd;
                c_ctx.c_addr = c_addr;

                thread recvThread([this, c_ctx]()
                                  { this->recvFromClient(c_ctx); });
                recvThread.detach();

                cout << "Connection: {ip: " << inet_ntoa(c_addr.sin_addr) << " port: " << ntohs(c_addr.sin_port) << "}\n";
            }
        }
    }

    void recvFromClient(client_ctx c_ctx)
    {
        struct sockaddr_in c_addr = c_ctx.c_addr;
        int connfd = c_ctx.connfd;

        char buffer[1000];

        while (1)
        {
            bzero(buffer, sizeof(buffer));
            if (recv(connfd, buffer, 1000, 0) > 0)
            {
                cout << "\n---Received--- \nSource: {ip: " << inet_ntoa(c_addr.sin_addr) << " port: " << ntohs(c_addr.sin_port) << "}\n";

                istringstream ss(buffer);
                string token;
                vector<string> tokens;
                while (getline(ss, token, ','))
                {
                    tokens.push_back(token);
                }

                // Check if the message type is INIT
                cout << "Request: " << tokens[0] << '\n';
                if (tokens.size() >= 4 && tokens[0] == "INIT")
                {
                    // Extract the values
                    peer_data pdata;
                    string username = tokens[1];
                    pdata.dir = tokens[2];
                    pdata.port = stoi(tokens[3]);

                    // Store files in a vector
                    pdata.files = vector<string>(tokens.begin() + 4, tokens.end());
                    pdata.ip = inet_ntoa(c_addr.sin_addr);
                    peers.insert(make_pair(username, pdata));
                    printPeers();
                }
                else if (tokens.size() == 1 && tokens[0] == "GET_U")
                {
                    string users = getAllUsernames();
                    cout << "Users: " << users << "\n\n";
                    send(connfd, users.c_str(), users.size(), 0);
                }
                else if (tokens.size() == 2 && tokens[0] == "GET_P")
                {
                    peer_data pdata = peers[tokens[1]];
                    cout << "IP: " << pdata.ip << "Port: " << pdata.port << "\n\n";
                    string data = pdata.ip + ", " + to_string(pdata.port);
                    send(connfd, data.c_str(), data.size(), 0);
                }
                else
                {
                    cout << "Invalid message type" << endl;
                }
            }
            else
            {
                cout << "Disconnected: {ip: " << inet_ntoa(c_addr.sin_addr) << " port: " << ntohs(c_addr.sin_port) << "}\n";
                close(connfd);
                break;
            }
        }
    }

    void printPeers()
    {
        for (const auto &entry : peers)
        {
            const std::string &username = entry.first;
            const peer_data &pdata = entry.second;
            cout << "Data: {\n\tUsername: " << username << endl;
            cout << "\tDirectory: " << pdata.dir << endl;
            cout << "\tPort: " << pdata.port << endl;
            cout << "\tFiles: ";
            for (const auto &file : pdata.files)
            {
                cout << file << ", ";
            }
            cout << "\n}\n\n";
            std::cout << std::endl;
        }
    }

    string getAllUsernames()
    {
        string result;

        for (const auto &pair : peers)
        {
            result += pair.first + ", ";
        }

        // Remove the trailing comma and space if there are any usernames
        if (!result.empty())
        {
            result.pop_back(); // Remove the last comma
            result.pop_back(); // Remove the last space
        }

        return result;
    }
};

int main()
{
    Server s1(80);
    s1.start();
    while (1)
    {
    }
    return 0;
}

void *sendToClient(void *arg)
{
    client_ctx *c_ctx = (client_ctx *)arg;
    struct sockaddr_in c_addr = c_ctx->c_addr;
    int connfd = c_ctx->connfd;

    const char *greet = "Greetings\n";

    send(connfd, greet, strlen(greet), 0);

    char buffer[1000];
    while (1)
    {
        cout << "Enter message: ";
        cin.getline(buffer, 1000);
        if (send(connfd, buffer, strlen(buffer), 0) < 0)
        {
            cin.clear();
            break;
        }
    }

    return NULL;
}
