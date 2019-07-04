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

struct request {
    char* ip, filename, path;
    char command;
};

int init_socket(unsigned short port, bool server, char* host) {
    int sockfd;
    char myname[MAX_HOSTNAME+1];
    struct sockaddr_in sa;
    struct hostent *hp;

    if (server) {
        gethostname(myname, MAX_HOSTNAME);
        hp = gethostbyname(myname);
    } else {
        hp = gethostbyname(host);
    }
    
    if (!hp) {
        ERR("error in gethostbyname()");
        return -1;
    }

    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = hp->h_addrtype;
    memcpy((char*)&sa.sin_addr, hp->h_addr, hp->h_length);
    sa.sin_port = htons(port); 

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        ERR("error in socket()");
        return -1;
    }

    if (server) {
        int len = sizeof(struct sockaddr_in);
        const struct sockaddr* addr = (struct sockaddr *)&sa;
        if (bind(sockfd, addr, len) < 0) {
            ERR("error in bind()");
            close(sockfd);
            return -1;
        }
        printf(SERVERS_BIND_IP_STR, inet_ntoa(sa.sin_addr));
        return sockfd;
    }
    const struct sockaddr* addr = (struct sockaddr *)&sa;
    if (connect(sockfd, addr, sizeof(sa)) < 0) {
        ERR("error in connect()");
        close(sockfd);
        return -1;
    }
    printf(CONNECTED_SUCCESSFULLY_STR);
    return sockfd;
}

int run_server(char* path, int port)
{
    int my_socket, remote_socket;
    if ((my_socket = init_socket(port, true, NULL)) < 0) {
        ERR("error in init_socket()");
        return -1;
    }
    listen(my_socket, MAX_QUEUE);
    printf(WAIT_FOR_CLIENT_STR);

    fd_set clients_fds, read_fds;
    FD_ZERO(&clients_fds);
    FD_SET(my_socket, &clients_fds);
    FD_SET(STDIN_FILENO, &clients_fds);

    bool running = true;
    while (running) {
        read_fds = clients_fds;
        if (select(MAX_CLIENTS+1, &read_fds, NULL, NULL, NULL) < 0) {
            //terminate_server();
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
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            //get_input();
        }
        else {
            //will check each client if it’s in readfds
            //and then receive a message from him
            //handle_clients();
        }
    }
}

int run_client(char* path, char* file, char* ip, unsigned short port, char mode)
{
    int my_socket;
    if ((my_socket = init_socket(port, false, ip)) < 0) {
        ERR("error in init_socket()");
        return -1;
    }
}

int read_write_data(int s, char *buf, int n, bool read_bool=true)
{
    /**
     * Reads or writes data from file descriptor to buffer or the opposite 
     * according to number of iterations to read/write which is defined in n.
     * read boolean default is to read (true), if false we write from buf to file.
    **/
    int bcount = 0;
    int br = 0;
    while (bcount < n) {
        br = read_bool ? read(s, buf, n-bcount) : write(s, buf, n-bcount); 
        if (br > 0) {
            bcount += br;
            buf += br;
        }
        if (br < 1) {
            return -1;
        }
    }
    return bcount;
}

int main(int argc, char **argv)
{
    /**
     * main function that reads the given arguments.
     * the arguemnts shuold be:
     * argv[0] = 'FileApp'
     * argv[1] = -s/-u/-d
     * (server/client)
     *      argv[2] = local_dir_path / local_path
     *      argv[3] = port_no / remote_name
     * (client only)
     *      argv[4] = port_no 
     *      argv[5] = server_ip 
    **/
    char mode = argv[1][1];
    unsigned short port_num;
    switch (mode)
    {
    case 's':
        port_num = (unsigned short) strtoul(argv[3], NULL, 0);
        run_server(argv[2], port_num);
        break;
    case 'u': 
    case 'd':
        port_num = (unsigned short) strtoul(argv[4], NULL, 0);
        run_client(argv[2], argv[3], argv[5], port_num, mode);
    default:
        fprintf(stderr, "Wrong switch inserted, supported switches are -s, -d, -u, you wrote %s", argv[1]);
    }
}