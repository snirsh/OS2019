#include "prints.h"
#include <cstring>
#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>

#define MSG(msg) std::cout << msg << std::endl
#define ERR(msg) std::cerr << msg << std::endl
#define MAX_CLIENTS 30
#define MAX_HOSTNAME 30
#define MAX_QUEUE 3
#define MAX_PATH 4095
#define LEN_IP 16
#define LEN_SIZE 16
#define LEN_FILENAME 255
#define BUF_SIZE 1024

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

int init_socket(unsigned short port, bool server, char* ip) {
    int sockfd;
    char myname[MAX_HOSTNAME+1];
    struct sockaddr_in sa;
    struct hostent *hp;

    if (server) {
        gethostname(myname, MAX_HOSTNAME);
        hp = gethostbyname(myname);
    } else {
        hp = gethostbyname(ip);
    }
    
    if (!hp) {
        ERR("error in gethostbyname()");
        return -1;
    }

    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = hp->h_addrtype;
    memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
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
    } else {
        const struct sockaddr* addr = (struct sockaddr *)&sa;
        if (connect(sockfd, addr, sizeof(sa)) < 0) {
            ERR("error in connect()");
            close(sockfd);
            return -1;
        }
        printf(CONNECTED_SUCCESSFULLY_STR);
        return sockfd;
    }
}

int run_server(char* path, int port)
{
    // open the socket
    int my_socket, remote_socket;
    if ((my_socket = init_socket(port, true, NULL)) < 0) {
        ERR("error in init_socket()");
        return -1;
    }
    listen(my_socket, MAX_QUEUE);

    // wait for connection
    printf(WAIT_FOR_CLIENT_STR);
    remote_socket = accept(my_socket, NULL, NULL);
    if (remote_socket < 0) {
        ERR("error in accept()");
        return -1;
    }

    // get from client: IP, mode, filename
    char ip[LEN_IP];
    char filename[LEN_FILENAME+1];
    char mode;
    read_write_data(remote_socket, ip, LEN_IP);
    read_write_data(remote_socket, &mode, 1);
    read_write_data(remote_socket, filename, LEN_FILENAME);
    printf(CLIENT_IP_STR, ip);
    printf(CLIENT_COMMAND_STR, mode);
    printf(FILENAME_STR, filename);

    // get from client: file size
    size_t size_u;
    read_write_data(remote_socket, (char*)&size_u, sizeof(size_t));
    if (size_u == -1) {
        printf(REMOTE_FILE_ERROR_STR);
        printf(FAILURE_STR);
        return -1;
    }

    // send to client: file size (0 if upload, -1 if file error)
    size_t size_d = 0;
    if (mode == 'd') {
        struct stat st;
        if (stat(path, &st) < 0) {
            size_d = -1;
        } else {
            size_d = st.st_size;
        }
    }
    read_write_data(remote_socket, (char*)&size_d, sizeof(size_t), false);
    if (size_d == -1) {
        printf(MY_FILE_ERROR_STR);
        printf(FAILURE_STR);
        return -1;
    }
    
    // check path and filename, send reply to client
    char full_path[MAX_PATH];
    strcpy(full_path, path);
    strcat(full_path, "/");
    strcat(full_path, filename);
    printf(FILE_PATH_STR, full_path);

    char reply = '0';
    if (strlen(filename) >= LEN_FILENAME || strstr(filename, "/")
        || strlen(full_path) >= MAX_PATH) {
        printf(FILE_NAME_ERROR_STR);
        printf(FAILURE_STR);
        reply = '1';
        write(remote_socket, &reply, 1);
        // goto ready;
    }
    write(remote_socket, &reply, 1);

    // upload/download the file
    printf("size_u:%zu, size_d:%zu\n", size_u, size_d);
    FILE *file;
    char buf[BUF_SIZE];
    size_t count = 0;
    size_t ret;

    if (mode == 'u') {
        file = fopen(full_path, "w+");
        char buf[BUF_SIZE];
        size_t count = 0;
        size_t ret;
        while (count < size_u) {
            ret = read_write_data(remote_socket, buf, BUF_SIZE);
            if (ret < 0) {
                printf(FAILURE_STR);
                return -1;
            }
            fprintf(file, "%s", buf);
            count += ret;
        }
        printf(SUCCESS_STR);
    }
    if (mode == 'd') {
        file = fopen(path, "r");
        while (count < size_d) {
            ret = fread(buf, 1, BUF_SIZE, file);
            if (ret < 0) {
                printf(FAILURE_STR);
                return -1;
            }
            ret = read_write_data(my_socket, buf, BUF_SIZE, false);
            count += ret;
        }
        printf(SUCCESS_STR);
    }
}

int run_client(char* path, char* file_name, char* ip, unsigned short port, char mode)
{
    // open the socket
    int my_socket;
    if ((my_socket = init_socket(port, false, ip)) < 0) {
        ERR("error in init_socket()");
        return -1;
    }

    // get the client IP
    char myname[MAX_HOSTNAME+1];
    char* my_ip;
    struct hostent *hp;
    struct sockaddr_in sa;
    gethostname(myname, MAX_HOSTNAME);
    hp = gethostbyname(myname);
    if (!hp) {
        ERR("error in gethostbyname()");
        return -1;
    }
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = hp->h_addrtype;
    memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
    my_ip = inet_ntoa(sa.sin_addr);

    // send to server: IP, mode, filename
    read_write_data(my_socket, my_ip, LEN_IP, false);
    read_write_data(my_socket, &mode, 1, false);
    read_write_data(my_socket, file_name, LEN_FILENAME, false);

    // send to server: file size (0 if download, -1 if file error)
    size_t size_u = 0;
    if (mode == 'u') {
        struct stat st;
        if (stat(path, &st) < 0) {
            size_u = -1;
        } else {
            size_u = st.st_size;
        }
    }
    read_write_data(my_socket, (char*)&size_u, sizeof(size_t), false);
    if (size_u == -1) {
        printf(MY_FILE_ERROR_STR);
        printf(FAILURE_STR);
        return -1;
    }

    // get from server: file size
    size_t size_d = 0;
    read_write_data(my_socket, (char*)&size_d, sizeof(size_t));
    if (size_d == -1) {
        printf(REMOTE_FILE_ERROR_STR);
        printf(FAILURE_STR);
        return -1;
    }

    // get reply from server
    char reply;
    read(my_socket, &reply, 1);
    if (reply == '1') {
        printf(FILE_NAME_ERROR_STR);
        printf(FAILURE_STR);
        return -1;
    }

    // upload/download the file
    printf("size_u:%zu, size_d:%zu\n", size_u, size_d);
    FILE *file;
    char buf[BUF_SIZE];
    size_t count = 0;
    size_t ret;
    
    if (mode == 'u') {
        file = fopen(path, "r");
        while (count < size_u) {
            ret = fread(buf, 1, BUF_SIZE, file);
            if (ret < 0) {
                printf(FAILURE_STR);
                return -1;
            }
            ret = read_write_data(my_socket, buf, BUF_SIZE, false);
            count += ret;
        }
        printf(SUCCESS_STR);
    }
    if (mode == 'd') {
        file = fopen(path, "w+");
        while (count < size_d) {
            ret = read_write_data(my_socket, buf, BUF_SIZE);
            if (ret < 0) {
                printf(FAILURE_STR);
                return -1;
            }
            fprintf(file, "%s", buf);
            count += ret;
        }
        printf(SUCCESS_STR);
    }
}

void running_loop()
{
    // set of socket descriptors
    MAX_CLIENTS = 1;
    fd_set clientfds;
    fd_set readfds;

    FD_ZERO(&clientfds);
    FD_SET(my_socket, &clientfds);
    FD_SET(STDIN_FILENO, &clientfds)

    while (true)
    {
        /**
         * server's run loop with fds
         */
        readfds = clientfds;

        activity = select(max_sd+1, &readfds, NULL, NULL, NULL);
        if(activity<0)
        {
            terminateServer();
            return -1;
        }
        printf(WAIT_FOR_CLIENT_STR);
        // incomming connection
        if(FD_ISSET(my_socket, &readfds))
        {
            // TODO: connect a new client
            if((new_socket = accept(my_socket, (struct sockaddr*) &address, (socklen_t*)addr_len)) < 0)
            {
                ERR("error in accept")
                return -1;
            }
            
            //informing user of connection
            printf(CONNECTED_SUCCESSFULLY_STR)
        }
        if(FD_ISSET(STDIN_FILENO, &readfds)){
            serverStdInput();
        }
        else
        {
            /* check every client if in readfds and recieve message from him */
        }
    }
    
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
        break;
    default:
        fprintf(stderr, "Wrong switch inserted, supported switches are -s, -d, -u, you wrote %s", argv[1]);
    }
}