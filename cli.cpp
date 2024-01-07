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
#include <filesystem>
#include <numeric>
#include <thread>
#include <atomic>

#define MIN_PORT 1
#define MAX_PORT 65535
#define SERVER_PORT 80
#define SERVER_IP "127.0.0.1"

using namespace std;

void printVector(const vector<string> &);
void *recvFromServer(void *);
void *sendToServer(void *);
int connect_server(int);
bool client_registeration(int);

class Client
{
private:
	string dir;
	vector<string> files;
	atomic<bool> chatSession;

	void input_data()
	{
		cout << "Username: ";
		getline(cin, username);

		do
		{
			cout << "Enter a directory: ";
			getline(cin, dir);

			// Check if the entered directory is valid
			if (filesystem::is_directory(dir))
				break; // Exit the loop if a valid directory is entered
			else
				cout << "Invalid directory. Please try again.\n";

		} while (true);

		cout << "Port: ";
		cin >> port;
	}

	int serverConnect()
	{
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd == -1)
		{
			perror("Socket creation failed : ");
			exit(-1);
		}

		struct sockaddr_in s_addr;
		s_addr.sin_family = AF_INET;
		s_addr.sin_port = htons(SERVER_PORT);
		inet_aton(SERVER_IP, &s_addr.sin_addr);

		if (connect(fd, (struct sockaddr *)&s_addr, sizeof(s_addr)) == -1)
		{
			perror("Connect failed on socket : ");
			exit(-1);
		}

		return fd;
	}

public:
	string username;
	int port;

	Client()
	{
		input_data();
		chatSession = false;
	}

	Client(string username, string dir, int port)
	{
		this->username = username;
		this->dir = dir;
		this->port = port;
		this->chatSession = false;
	}

	void serverRegister()
	{
		int fd = serverConnect();

		if (!fetchFiles())
		{
			perror("File fetching failed : ");
			exit(-1);
		}

		cout << "Shared files: ";
		printVector(files);

		string data = "INIT," + username + ',' + dir + ',' + to_string(this->port) + ',' + filesToString();
		send(fd, data.c_str(), data.size(), 0);
		cout << "Registered with server!\n\n";

		close(fd);
	}

	string fetchPeerData(string p_username)
	{
		int fd = serverConnect();

		if (!fetchFiles())
		{
			perror("File fetching failed : ");
			exit(-1);
		}

		char buffer[1000];
		bzero(buffer, sizeof(buffer));

		string data = "GET_P," + p_username;
		send(fd, data.c_str(), data.size(), 0);
		cout << "Requested for data of " << p_username << "!\n";

		if (recv(fd, buffer, 1000, 0) > 0)
			cout << "Peer data: " << buffer << "\n\n";

		return buffer;
		close(fd);
	}

	string fetchUsernames()
	{
		int fd = serverConnect();

		char users[1000];
		bzero(users, sizeof(users));

		string data = "GET_U";
		send(fd, data.c_str(), data.size(), 0);
		cout << "Requested for usernames!\n";

		if (recv(fd, users, 1000, 0) > 0)
			cout << "Usernames: " << users << "\n\n";

		close(fd);
		return users;
	}

	void updateFiles(const vector<string> &files)
	{
		this->files = files;
	}

	string filesToString()
	{
		const char separator = ',';
		return accumulate(files.begin(), files.end(), string(),
						  [separator](const string &a, const string &b)
						  {
							  return a + (a.empty() ? "" : string(1, separator)) + b;
						  });
	}

	bool fetchFiles()
	{
		try
		{
			for (const auto &entry : filesystem::directory_iterator(dir))
			{
				if (filesystem::is_regular_file(entry.path()))
					files.push_back(entry.path().filename());
			}
		}
		catch (const filesystem::filesystem_error &e)
		{
			cerr << "Error accessing the directory: " << e.what() << std::endl;
			return false;
		}
		return true;
	}

	vector<string> getFiles()
	{
		return files;
	}

	void peerConnect(string username)
	{
		string data = fetchPeerData(username);

		istringstream ss(data);
		string token;
		vector<string> tokens;
		while (getline(ss, token, ','))
			tokens.push_back(token);

		string p_ip = tokens[0];
		int p_port = stoi(tokens[1]);

		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd == -1)
		{
			perror("Socket creation failed : ");
			exit(-1);
		}

		struct sockaddr_in p_addr;
		p_addr.sin_family = AF_INET;
		p_addr.sin_port = htons(p_port);
		inet_aton(p_ip.c_str(), &p_addr.sin_addr);

		if (connect(fd, (struct sockaddr *)&p_addr, sizeof(p_addr)) == -1)
		{
			perror("Connect failed on socket : ");
			exit(-1);
		}
		chatSession = true;

		thread sendPeerThread([this, fd]()
							  { this->sendToPeer(fd); });
		thread recvThread([this, fd]()
						  { this->recvFromPeer(fd); });

		recvThread.join();
		sendPeerThread.join();
		close(fd);

		cout << "Chat session closed!\n";
	}

	void start()
	{
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd == -1)
		{
			perror("Socket creation failed : ");
			exit(-1);
		}

		struct sockaddr_in c_addr;
		c_addr.sin_addr.s_addr = INADDR_ANY;
		c_addr.sin_family = AF_INET;
		c_addr.sin_port = htons(port);

		cout << "listening on port: " << port << '\n';

		if (bind(fd, (struct sockaddr *)(&c_addr), sizeof(c_addr)) == -1)
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
		struct sockaddr_in p_addr;
		socklen_t paddr_len = sizeof(p_addr);

		while (1)
		{
			int connfd = accept(fd, (struct sockaddr *)&p_addr, &paddr_len);
			if (connfd < 0)
			{
				perror("accept failed on socket : ");
				exit(-1);
			}
			else
			{
				// client_ctx c_ctx;
				// c_ctx.connfd = connfd;
				// c_ctx.c_addr = c_addr;

				cout << "Connection: {ip: " << inet_ntoa(p_addr.sin_addr) << " port: " << ntohs(p_addr.sin_port) << "}\n";
				chatSession = true;

				thread recvThread([this, connfd]()
								  { this->recvFromPeer(connfd); });
				thread sendPeerThread([this, connfd]()
									  { this->sendToPeer(connfd); });
				recvThread.join();
				sendPeerThread.join();

				cout << "Chat session closed!\n";
			}
		}
	}

	void recvFromPeer(int connfd)
	{
		char buffer[1000];

		while (chatSession)
		{
			bzero(buffer, sizeof(buffer));
			if (recv(connfd, buffer, 1000, 0) > 0)
			{
				cout << buffer << '\n';
			}
			else
			{
				chatSession = false;
				break;
			}
		}
		close(connfd);
	}

	void sendToPeer(int connfd)
	{
		char buffer[1000];

		while (chatSession)
		{
			cout << "Enter Message : ";
			cin.getline(buffer, 1000);

			if (strcmp(buffer, "!exit") == 0)
			{
				cout << "exiting...\n";
				chatSession = false;
				break;
			}

			send(connfd, buffer, strlen(buffer), 0);
		}
		close(connfd);
	}
};

int main()
{

	int op;
	cout << "option: ";
	cin >> op;
	cin.ignore();
	if (op == 1)
	{
		Client c1("hello", "../../Desktop", 12);
		c1.serverRegister();
		c1.start();
		// c1.fetchUsernames();
	}
	else if (op == 2)
	{
		Client c2("hey", ".", 44);
		c2.serverRegister();
		c2.fetchUsernames();
		// c2.fetchPeerData("hello");

		c2.peerConnect("hello");
	}

	while (1)
	{
	}

	//  pthread_t handler;
	//   pthread_create(&handler, NULL, recvFromServer, (void *)server_fd);
	//    pthread_create(&handler, NULL, sendToServer, (void *)server_fd);

	// while (1)
	// {
	// }

	return 0;
}

void printVector(const vector<string> &files)
{
	for (const auto &element : files)
		cout << element << ", ";

	cout << '\n';
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

	return NULL;
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

	return NULL;
}