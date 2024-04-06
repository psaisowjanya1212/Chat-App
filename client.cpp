#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <pthread.h>
#include <fstream>
#include <fcntl.h>
#include <sys/select.h>

using namespace std;
const unsigned MAXBUFLEN = 512;
int sockfd;
bool logged_in = false;

void *process_connection(void *arg) {
	int n;
	char buf[MAXBUFLEN];
	pthread_detach(pthread_self());
	while (1) {
		n = read(sockfd, buf, MAXBUFLEN);
		if (n <= 0) {
			if (n == 0) {
				cout << "server closed" << endl;
			} else {
				cout << "something wrong" << endl;
			}
			close(sockfd);
			exit(1);
		}
		buf[n] = '\0';
		cout << buf << endl;
	}
}

int main(int argc, char **argv) {
	int rv, flag;
	struct addrinfo hints, *res, *ressave;
	pthread_t tid;
	string configuration_file;
	string servhost;
	unsigned servport = 0;

	if (argc != 2) {
        cout << "Usage: chat_client <configuration_file>" << endl;
        exit(1);
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
        if (keyword == "servhost") {
            servhost = value;
        } else if (keyword == "servport") {
           servport = std::atoi(value.c_str());
        }
    }
    infile.close();

    cout << "Server host: " << servhost << endl;
    cout << "Server port: " << servport << endl;

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
  if ((rv = getaddrinfo(servhost.c_str(), std::to_string(servport).c_str(), &hints, &res)) != 0) {
      cout << "getaddrinfo wrong: " << gai_strerror(rv) << endl;
      exit(1);
  }

	ressave = res;
	flag = 0;
	do {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0) 
			continue;
		if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0) {
			flag = 1;
			break;
		}
		close(sockfd);
	} while ((res = res->ai_next) != NULL);
	freeaddrinfo(ressave);

	if (flag == 0) {
		fprintf(stderr, "cannot connect\n");
		exit(1);
	}

	pthread_create(&tid, NULL, &process_connection, NULL);

	/* set non-blocking mode for stdin */
	fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

	/* set up select */
	fd_set read_fds;
    fd_set master_fds;
    int fdmax = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;
    FD_ZERO(&master_fds);
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &master_fds);
    FD_SET(STDIN_FILENO, &master_fds);

    string oneline;
    bool isLogin = false;
    int n;
    char buf[MAXBUFLEN];
    while (1) {
        read_fds = master_fds;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        }

        /* check for input from the server */
        if (FD_ISSET(sockfd, &read_fds)) {
            n = read(sockfd, buf, MAXBUFLEN);
            if (n <= 0) {
                if (n == 0) {
                    cout << "server closed" << endl;
                } else {
                    cout << "something wrong" << endl;
                }
                close(sockfd);
                exit(1);
            }
            buf[n] = '\0';
            cout << buf << endl;
        }

        /* check for input from stdin */
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            getline(cin, oneline);
            string message(oneline);
            if(strcmp(string(oneline).substr(0,5).c_str(), "login") == 0) {
              if(isLogin){
                cout << "Already logged in.\n";
                continue;
              }
	      else if(oneline.size() > 5){
		 if(oneline.substr(6) != " "){
                 isLogin=true;
              }
              else
		 cout<<"Please enter username";
		}
            /*else if(oneline.size() == 6) {
		if(oneline.substr(6) == " ")
			cout<<"Please enter username\n";
		}*/
	    }
            if(strcmp(string(oneline).substr(0,4).c_str(), "exit") == 0) {
              if(isLogin){
                cout << "You need to logout first to exit.\n";
              }
              else{
                cout << "Client exited.\n";
                close(sockfd);
                exit(0);
              }
            }
            if(strcmp(string(oneline).substr(0,6).c_str(), "logout") == 0) {
              isLogin=false;
            }
            write(sockfd, message.c_str(), strlen(message.c_str())+1);
          }          
    }
}
