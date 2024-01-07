#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <pthread.h>

#include <vector>
#include <sstream>
#include <map>

using namespace std;

struct client_ctx
{
    int connfd;
    sockaddr_in c_addr;
};

void *recvFromClient(void *);
void *sendToClient(void *);

int main()
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

    struct sockaddr_in c_addr;
    socklen_t cliaddr_len = sizeof(c_addr);
    pthread_t handler;
    cout << "Starting Server...\n\n";

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

            // pthread_create(&handler, NULL, sendToClient, &c_ctx);
            pthread_create(&handler, NULL, recvFromClient, &c_ctx);

            cout << "Connection: {ip: " << inet_ntoa(c_addr.sin_addr) << " port: " << ntohs(c_addr.sin_port) << "}\n";
        }
    }
    return 0;
}

void *recvFromClient(void *arg)
{
    client_ctx *c_ctx = (client_ctx *)arg;
    struct sockaddr_in c_addr = c_ctx->c_addr;
    int connfd = c_ctx->connfd;

    char buffer[1000];

    while (1)
    {
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
            if (tokens.size() >= 4 && tokens[0] == "INIT")
            {
                // Extract the values
                string username = tokens[1];
                string dir = tokens[2];
                int port = stoi(tokens[3]);

                // Store files in a vector
                vector<string> files(tokens.begin() + 4, tokens.end());

                // Display the extracted values
                cout << "Data: {\n\tUsername: " << username << endl;
                cout << "\tDirectory: " << dir << endl;
                cout << "\tPort: " << port << endl;
                cout << "\tFiles: ";
                for (const auto &file : files)
                {
                    cout << file << ", ";
                }
                cout << "\n}\n\n";
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
    return NULL;
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
