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

public:
	string username;
	int port;

	Client()
	{
		input_data();
	}

	Client(string username, string dir, int port)
	{
		this->username = username;
		this->dir = dir;
		this->port = port;
	}

	void serverConnect(string server_ip, int server_port)
	{
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd == -1)
		{
			perror("Socket creation failed : ");
			exit(-1);
		}

		struct sockaddr_in s_addr;
		s_addr.sin_family = AF_INET;
		s_addr.sin_port = htons(server_port);
		inet_aton(server_ip.c_str(), &s_addr.sin_addr);

		if (connect(fd, (struct sockaddr *)&s_addr, sizeof(s_addr)) == -1)
		{
			perror("Connect failed on socket : ");
			exit(-1);
		}

		if (!fetchFiles())
		{
			perror("File fetching failed : ");
			exit(-1);
		}

		cout << "Shared files: ";
		printVector(files);

		string data = "INIT," + username + ',' + dir + ',' + to_string(this->port) + ',' + filesToString();
		send(fd, data.c_str(), data.size(), 0);
		cout << "Registered with server!\n";

		close(fd);
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
};

int main()
{
	// int server_fd = connect_server(SERVER_PORT);
	//  client_registeration(server_fd);

	Client c1("hello", "../../Desktop", 12);
	c1.serverConnect(SERVER_IP, SERVER_PORT);

	// Client c2("hey", ".", 44);
	// c2.serverConnect(SERVER_IP, SERVER_PORT);

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