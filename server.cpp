#include <ios>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <iosfwd>
#include <vector>
#include <algorithm>
#define BUFFER_SIZE 1024


using namespace std;

const unsigned MAXBUFLEN = 512;

struct client_info {
    int sockfd;
    string username;
};
string username;
vector<client_info> clients;

bool logged_in = false;

void handle_logout(int client_socket, const std::string& client_name) {
    bool login = false;
    std::string message;
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it->sockfd == client_socket) {
            if (it->username == client_name) {
                login = true;
            }
            clients.erase(it);
            break;
        }
    }

    if (login) {
        message = client_name + " has logged out\n";
        std::cout << message;

        for (auto it = clients.begin(); it != clients.end(); ++it) {
            if (it->sockfd == client_socket) {
                if (send(client_socket, message.c_str(), message.length(), 0) < 0) {
                    perror("send failed\n");
                    return;
                }
                it->username = "";
                it->sockfd = -1;
                logged_in = false;
                break;
            }
        }
    } else {
        message = "Error: You must be logged in to use the logout command\n";
        send(client_socket, message.c_str(), message.length(), 0);
        return;
    }
}

void* process_connection(void* arg) {
    int sockfd = *(int*)arg;
    char buffer[BUFFER_SIZE];

    /* Receive login command and add client to clients vector */
    bool login_required = true;

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);

        int ret = recv(sockfd, buffer, BUFFER_SIZE, 0);

        if (ret <= 0) {
            cout << "Client disconnected: " << username << endl;

            /* Remove client from clients vector */
            for (auto it = clients.begin(); it != clients.end(); ++it) {
                if (it->sockfd == sockfd) {
                    // it->logged_in = false;
                    logged_in = false;
                    break;
                }
            }
            close(sockfd);
            pthread_exit(NULL);
        }

        /* Handle chat command */
        string chat_cmd = string(buffer);
        bool already_logged_in = false;
        if(chat_cmd.find("logout") != std::string::npos) {
          string logout_user;
          for(auto it = clients.begin(); it != clients.end(); ++it){
            if(it -> sockfd == sockfd)
            {
              logout_user = it->username;
            }
          }
          handle_logout(sockfd, logout_user);
        }
        else if (chat_cmd.substr(0, 6) == "login ") {
          std::string username = chat_cmd.substr(6);
          /*Check if user is already logged in */
          for (const auto& client : clients) {
              if (client.username == username) {
                  already_logged_in = true;
                  break;
              }
          }
          /* If user is already logged in, notify the client and don't add to clients list */
          if (already_logged_in) {
              std::string message = "Error: User " + username + " is already logged in.\n";
              if (send(sockfd, message.c_str(), message.length(), 0) < 0) {
                  perror("send failed\n");
              }
          }
          client_info new_client;
          new_client.sockfd = sockfd;
          new_client.username = username;
          clients.push_back(new_client);
          std::cout << "New client logged in: " << username << std::endl;
          logged_in = true;
        }
        else if (chat_cmd.substr(0, 5) == "chat ") {
          string message = chat_cmd.substr(5);
          string sender;
          if (message.size() == 0) {
              continue;
          }
          for (const client_info& c : clients)
          {
            if (c.sockfd == sockfd) {
                sender = c.username;
            }
          }
          string recipient;
          size_t pos = message.find(" ");
          if (pos != string::npos) {
              recipient = message.substr(0, pos);
              message = message.substr(pos + 1);
          }
          if (recipient.empty()) {
            /* Send message to all clients */
              for (const client_info& c : clients)
              {
                  if (c.sockfd == sockfd) {
                      continue;
                  }
                  if(message.substr(0,1) != "@"){
                    string send_message = sender + " >> " + message;
                    send(c.sockfd, send_message.c_str(), send_message.size(), 0);
                  }
                  else{
                      string send_message = "please enter msg";
                      send(sockfd, send_message.c_str(), send_message.size(), 0);
                  }
              }
          }
          else if (recipient.substr(0,1) != "@")
          {
            for (const client_info& c : clients)
            {
              if(recipient != "@"){
                string send_message = sender + " >> " + recipient + " " + message;
                send(c.sockfd, send_message.c_str(), send_message.size(), 0);
              }
            }
          }
          else if (recipient.substr(0, 1) == "@") {
            /* Send message to intended recipient */
            bool recipient_found = false;
            for (const client_info& c : clients) {
                if (c.username == recipient.substr(1)) {
                    recipient_found = true;
                    if(!message.empty())
                    {
                      string send_message = sender + " >> " + message;
                      send(c.sockfd, send_message.c_str(), send_message.size(), 0);
                      break;
                    }
                    else {
                      string send_message = "Please enter message to send to the user";
                      send(c.sockfd, send_message.c_str(), send_message.size(), 0);
                    }
                }
            }
              if (!recipient_found) {
                  string send_message = "User " + recipient.substr(1) + " not found.";
                  send(sockfd, send_message.c_str(), send_message.size(), 0);
              }
            } else {
                string send_message = "Recipient username must start with '@'.";
                send(sockfd, send_message.c_str(), send_message.size(), 0);
            }
          }
          else if (chat_cmd.substr(0, 4) == "exit"){
            if(!logged_in) {
                close(sockfd);
            }
            else{
              string send_message = "First logout to exit";
              send(sockfd, send_message.c_str(), send_message.size(), 0);
            }
          }
          else {
              string send_message = "Use chat, login, logout.";
              send(sockfd, send_message.c_str(), send_message.size(), 0);
          }
        }
    }

int main(int argc, char *argv[]) {
        vector<int> clients;
    int serv_sockfd, cli_sockfd, *sock_ptr;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t sock_len;
    pthread_t tid;
    string configuration_file;
    unsigned port = 0;

    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <configuration_file>" << endl;
        return 1;
    }
    configuration_file = argv[1];

    ifstream infile(configuration_file.c_str());
    if (!infile) {
        cout << "Error opening file " << configuration_file << endl;
        return 1;
    }

    string line;
    while (getline(infile, line)) {
        size_t pos = line.find(':');
        if (pos == string::npos) {
            continue;
        }
        string keyword = line.substr(0, pos);
        string value = line.substr(pos + 1);
        if (keyword == "port") {
                        port = std::atoi(value.c_str());
            /*port = stoi(value);*/
        }
    }
    infile.close();

   if (port == 0) {
    serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero((void*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(0); // system will assign any unused port number

    bind(serv_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    listen(serv_sockfd, 5);

    sock_len = sizeof(serv_addr);
    getsockname(serv_sockfd, (struct sockaddr *)&serv_addr, &sock_len);

    cout << "server started on port = " << ntohs(serv_addr.sin_port) << endl;

    for (; ;) {
        sock_len = sizeof(cli_addr);
        cli_sockfd = accept(serv_sockfd, (struct sockaddr *)&cli_addr, &sock_len);

        cout << "remote client IP = " << inet_ntoa(cli_addr.sin_addr);
        cout << ", port = " << ntohs(cli_addr.sin_port) << endl;

        sock_ptr = (int *)malloc(sizeof(int));
        *sock_ptr = cli_sockfd;
                clients.push_back(cli_sockfd);

        pthread_create(&tid, NULL, &process_connection, (void *)sock_ptr);
    }
}
 else if (port > 0) {
    serv_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero((void*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    bind(serv_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    listen(serv_sockfd, 5);

    cout << "Server, the portstarted on = " << ntohs(serv_addr.sin_port) << endl;

    for (; ;) {
        sock_len = sizeof(cli_addr);
        cli_sockfd = accept(serv_sockfd, (struct sockaddr *)&cli_addr, &sock_len);

        cout << "remote client IP == " << inet_ntoa(cli_addr.sin_addr);
        cout << ", port == " << ntohs(cli_addr.sin_port) << endl;

        sock_ptr = (int *)malloc(sizeof(int));
        *sock_ptr = cli_sockfd;

        pthread_create(&tid, NULL, &process_connection, (void
                    *)sock_ptr);
    }
} else {
    cout << "Invalid port number specified in configuration file." << endl;
    return 1;
}
    return 0;
}
