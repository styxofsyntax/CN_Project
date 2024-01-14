#ifndef CLIENT_H
#define CLIENT_H

#include <vector>
#include <string>
#include <atomic>

void printVector(const std::vector<std::string> &);
std::vector<std::string> stringToTokens(std::string);

#define MIN_PORT 1
#define MAX_PORT 65535
#define SERVER_PORT 80
#define SERVER_IP "127.0.0.1"

class Client
{
private:
    std::string dir;
    std::vector<std::string> files;
    std::atomic<bool> chatSession;

    void input_data();
    int serverConnect();

public:
    std::string username;
    int port;

    Client();
    Client(std::string username, std::string dir, int port);
    void serverRegister();
    std::string fetchPeerData(std::string p_username);
    std::string fetchUsernames();
    void updateFiles(const std::vector<std::string> &files);
    std::string filesToString();
    bool fetchFiles();
    void fetchFilenamesFromServer();
    void fetchUserFilenamesFromServer(std::string username);
    std::vector<std::string> getFiles();
    int peerConnect(std::string p_ip, int p_port);
    void peerChat(std::string username);
    void start();
    void invokeAccept(int fd);
    void recvChatFromPeer(int connfd);
    void sendChatToPeer(int connfd);
};

#endif