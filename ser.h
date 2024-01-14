#include <netinet/in.h>
#include <string>
#include <vector>
#include <map>

#define SERVER_PORT = 80;

struct client_ctx
{
    int connfd;
    sockaddr_in c_addr;
};

struct peer_data
{
    std::string dir, ip;
    int port;
    std::vector<std::string> files;
};

std::string vectorToString(const std::vector<std::string> &files);

class Server
{
private:
    int port, fd;
    std::map<std::string, peer_data> peers;

public:
    Server(int port);
    void start();
    void invokeAccept(int fd);
    void recvFromClient(client_ctx c_ctx);
    void printPeers();
    std::string registerUser(std::string username, peer_data pdata);
    std::string getPeerData(std::string username);
    std::string getAllUsernames();
    std::string getAllFilenames();
    std::string getUserFilenames(std::string username);
};
