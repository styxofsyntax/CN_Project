#include <iostream>
#include "cli.h"

void userSelect(Client &);

int main()
{
    Client clientObj;
    if (!clientObj.serverRegister())
        return -1;

    while (1)
    {
        while (clientObj.blockUI)
        {
        }
        int op;
        std::cout << "Choose option:\n1. View Users\n2. Select User\n3. View All Files\n0. Exit\n";
        std::cin >> op;
        std::cout << "\n";
        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        switch (op)
        {
        case 1:
        {
            std::cout << "Requested for usernames!\n";
            std::vector<std::string> users = clientObj.fetchUsernames();

            std::cout << "Users: ";
            if (users.size() != 0)
            {
                for (const auto &username : users)
                    if (username != clientObj.username)
                        std::cout << username << ", ";

                std::cout << "\n\n";
            }

            break;
        }
        case 2:
        {
            std::cin.ignore();
            userSelect(clientObj);
            break;
        }
        case 3:
            std::cout << clientObj.fetchFilenamesFromServer() << "\n\n";
            break;
        case 9:
            if (clientObj.chatSession)
                clientObj.blockUI = true; // toggle on to indicate backend we accept request
            break;
        case 0:
            return 0;
        }
    }
}

void userSelect(Client &clientObj)
{
    std::string username;
    while (1)
    {
        std::cout << "Username to select: ";
        getline(std::cin, username);
        std::cout << std::endl;

        if (clientObj.userExists(username) &&
            username != clientObj.username) // if username exists on server
                                            // and is not clients own name
            break;
        if (username == "!exit")
        {
            std::cout << "exiting\n\n";
            return;
        }

        std::cout << "User not found!\n\n";
    }

    while (1)
    {
        while (clientObj.blockUI)
        {
        }

        int op;
        std::cout << "---Selected User: " << username << "---\n";
        std::cout << "Choose option:\n1. View IP/Port\n2. View Filenames\n3. Chat\n4. Fetch File\n0. Exit\n";
        std::cin >> op;
        std::cout << std::endl;

        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        switch (op)
        {
        case 1:
        {
            std::cout << "Requested for data of " << username << "!\n";
            std::string data = clientObj.fetchPeerData(username);
            std::vector<std::string> tokens = stringToTokens(data);

            if (tokens[0] == "ERR")
            {
                std::cout << "ERROR: " << tokens[1] << "\n\n";
                return;
            }

            std::cout << "IP: " << tokens[1] << " Port: " << tokens[2] << "\n\n";
        }
        break;
        case 2:
            std::cout << clientObj.fetchUserFilenamesFromServer(username) << "\n\n";
            break;
        case 3:
        {
            clientObj.peerChat(username);
            break;
        }
        case 9:
            if (clientObj.chatSession)
                clientObj.blockUI = true; // toggle on to indicate backend we accept request
            break;
        case 0:
            return;
        }
    }
}