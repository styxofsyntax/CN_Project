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

#define MIN_PORT 1
#define MAX_PORT 65535
#define SERVER_PORT 80

using namespace std;

void *recvFromServer(void *);
void *sendToServer(void *);
int connect_server(int);
bool client_registeration(int);

int main()
{
	int server_fd = connect_server(SERVER_PORT);
	client_registeration(server_fd);

	pthread_t handler;
	// pthread_create(&handler, NULL, recvFromServer, (void *)server_fd);
	//  pthread_create(&handler, NULL, sendToServer, (void *)server_fd);

	while (1)
	{
	}

	return 0;
}

int connect_server(int port)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
	{
		perror("Socket creation failed : ");
		exit(-1);
	}

	struct sockaddr_in s_addr;
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(80);
	inet_aton("127.0.0.1", &s_addr.sin_addr);

	if (connect(fd, (struct sockaddr *)&s_addr, sizeof(s_addr)) == -1)
	{
		perror("Connect failed on socket : ");
		exit(-1);
	}

	return fd;
}

bool client_registeration(int fd)
{
	client_data data;

	cout << "Username: ";
	cin.getline(data.username, sizeof(data.username) / sizeof(data.username[0]));

	cout << "Port: ";
	cin >> data.port;
	cin.ignore();

	cout << "Directory: ";
	cin.getline(data.dir, sizeof(data.dir) / sizeof(data.dir[0]));

	send(fd, (char *)&data, sizeof(data), 0);
}

void *recvFromServer(void *arg)
{
	int64_t fd = (int64_t)arg;
	char buffer[1000];

	while (1)
	{
		bzero(buffer, sizeof(buffer));
		if (recv(fd, buffer, 1000, 0) > 0)
		{
			cout << "from server: " << buffer << "\n";
		}
		else
		{
			break;
		}
	}
}

void *sendToServer(void *arg)
{
	int64_t fd = (int64_t)arg;
	char buffer[1000];

	while (1)
	{
		cout << "Enter Message : ";
		cin.getline(buffer, 1000);
		send(fd, buffer, strlen(buffer), 0);
	}
}