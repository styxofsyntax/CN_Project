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

#include "common.cpp"

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

    client_data c_data;

    while (1)
    {
        if (recv(connfd, (char *)&c_data, sizeof(client_data), 0) > 0)
        {
            cout << "Received : {ip: " << inet_ntoa(c_addr.sin_addr) << " port: " << ntohs(c_addr.sin_port) << "}\n";
            cout << "Username: " << c_data.username << "\nPort: " << c_data.port << "\nDirectory: " << c_data.dir << '\n';
        }
        else
        {
            cout << "Disconnected: {ip: " << inet_ntoa(c_addr.sin_addr) << " port: " << ntohs(c_addr.sin_port) << "}\n";
            close(connfd);
            break;
        }
    }
}

void *sendToClient(void *arg)
{
    client_ctx *c_ctx = (client_ctx *)arg;
    struct sockaddr_in c_addr = c_ctx->c_addr;
    int connfd = c_ctx->connfd;

    char *greet = "Greetings\n";

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
}
