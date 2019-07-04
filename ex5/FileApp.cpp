#include "prints.h"
#include <iostream>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MSG(msg) std::cout << msg << std::endl
#define ERR(msg) std::cerr << msg << std::endl
#define MAX_CLIENTS 30
#define MAX_HOSTNAME 30
#define MAX_QUEUE 3

typedef app_args;
struct request {

};

int main(int argc, char **argv)
{
    MSG(argv[0]);
    MSG(argv[1]);
    MSG(argv[2]);
    MSG(argv[3]);
    MSG(argv[4]);
    MSG(argv[5]);
    return 0;
}

int init_socket(unsigned short port, bool server) {
    int sockfd;
    char myname[MAX_HOSTNAME+1];
    struct sockaddr_in sa;
    struct hostent *hp;

    gethostname(myname, MAX_HOSTNAME);
    hp = gethostbyname(myname);
    if (!hp) {
        ERR("error in gethostbyname()");
        return -1;
    }

    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = hp->h_addrtype;
    memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
    sa.sin_port= htons(port); 

    if (sockfd = socket(AF_INET, SOCK_STREAM, 0)) {
        ERR("error in socket()");
        return -1;
    }
    int len = sizeof(struct sockaddr_in);
    const struct sockaddr* addr = (struct sockaddr *)&sa;
    if (bind(sockfd, addr, len) < 0) {
        ERR("error in bind()");
        close(sockfd);
        return -1;
    }
    if (server) {
        printf(SERVERS_BIND_IP_STR, inet_ntoa(sa.sin_addr));
    }
    return sockfd;
}

int run_server(std::string path, int port)
{
    int my_socket, remote_socket;
    if (my_socket = init_socket(port, true) < 0) {
        ERR("error in init_socket()");
        return -1;
    }
    listen(my_socket, MAX_QUEUE);
    printf(WAIT_FOR_CLIENT_STR);

    // read/write()

    fd_set clients_fds, read_fds;
    FD_ZERO(&clients_fds);
    FD_SET(my_socket, &clients_fds);
    FD_SET(STDIN_FILENO, &clients_fds);

    bool running = true;
    while (running) {
        read_fds = clients_fds;
        if (select(MAX_CLIENTS+1, &read_fds, NULL, NULL, NULL) < 0) {
            terminate_server();
            return -1;
        }
        if (FD_ISSET(my_socket, &read_fds)) {
            remote_socket = accept(my_socket, NULL, NULL);
            if (remote_socket < 0) {
                ERR("error in accept()");
                return -1;
            }
            FD_SET(remote_socket, &clients_fds);
    }
        }
            if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            get_input();
        }
        else {
            //will check each client if it’s in readfds
            //and then receive a message from him
            handle_clients();
        }
    }
}