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
#define MAX_CLIENTS 30

int init_socket(unsigned short port, bool server, char* ip)
{
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

int write_to_file(int socket, FILE* file, size_t size)
{
    char buf;
    size_t count = 0;
    while (count < size) {
        if (read(socket, &buf, 1) < 0) {
            return -1;
        }
        if (fputc(buf, file) < 0) {
            return -1;
        }
        count++;
    }
    fclose(file);
    return 0;
}

int read_from_file(int socket, FILE* file, size_t size)
{   
    char buf;
    size_t count = 0;
    while (count < size) {
        if ((buf = fgetc(file)) < 0) {
            return -1;
        }
        if (write(socket, &buf, 1) < 0) {
            return -1;
        }
        count++;
    }
    fclose(file);
    return 0;
}

int handle_clients(int socket, char* path, int port)
{
    // get from client: IP, mode, filename
    char ip[LEN_IP];
    char filename[LEN_FILENAME+1];
    char mode;
    read_write_data(socket, ip, LEN_IP);
    read_write_data(socket, &mode, 1);
    read_write_data(socket, filename, LEN_FILENAME);
    printf(CLIENT_IP_STR, ip);
    printf(CLIENT_COMMAND_STR, mode);
    printf(FILENAME_STR, filename);

    // get full path
    char full_path[MAX_PATH];
    strcpy(full_path, path);
    strcat(full_path, "/");
    strcat(full_path, filename);
    printf(FILE_PATH_STR, full_path);

    // check path and filename, send reply to client
    char reply = '0';
    if (strlen(filename) >= LEN_FILENAME || strstr(filename, "/")
        || strlen(full_path) >= MAX_PATH) {
        printf(FILE_NAME_ERROR_STR);
        reply = '1';
        write(socket, &reply, 1);
        return -1;
    }
    write(socket, &reply, 1);

    // get from client: file size
    size_t size_u = 0;
    read_write_data(socket, (char*)&size_u, sizeof(size_t));
    if (!size_u && mode == 'u') {
        printf(REMOTE_FILE_ERROR_STR);
        return -1;
    }

    // send to client: file size (0 if file error)
    size_t size_d = 0;
    if (mode == 'd') {
        struct stat st;
        if (fopen(full_path, "r") && !stat(full_path, &st)) {
            size_d = st.st_size;
        }
    }
    read_write_data(socket, (char*)&size_d, sizeof(size_t), false);
    if (!size_d && mode == 'd') {
        printf(MY_FILE_ERROR_STR);
        return -1;
    }
    
    // upload/download the file
    FILE* file;
    if (mode == 'u') {
        file = fopen(full_path, "w+");
        if (write_to_file(socket, file, size_u) < 0) {
                return -1;
        }
        return 0;
    }
    if (mode == 'd') {
        file = fopen(full_path, "r");
        if (read_from_file(socket, file, size_d) < 0) {
            return -1;
        }
        return 0;
    }
    return 0;
}

int run_client(char* path, char* file_name, char* ip, unsigned short port, char mode)
{
    // open the socket
    int my_socket;
    if ((my_socket = init_socket(port, false, ip)) < 0) {
        ERR("error in init_socket()");
        printf(FAILURE_STR);
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
        printf(FAILURE_STR);
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

    // get reply from server
    char reply;
    read(my_socket, &reply, 1);
    if (reply == '1') {
        printf(FILE_NAME_ERROR_STR);
        printf(FAILURE_STR);
        return -1;
    }

    // send to server: file size (0 if file error)
    size_t size_u = 0;
    if (mode == 'u') {
        struct stat st;
        if (fopen(path, "r") && !stat(path, &st)) {
            size_u = st.st_size;  
        }
    }
    read_write_data(my_socket, (char*)&size_u, sizeof(size_t), false);
    if (!size_u && mode == 'u') {
        printf(MY_FILE_ERROR_STR);
        printf(FAILURE_STR);
        return -1;
    }

    // get from server: file size
    size_t size_d = 0;
    read_write_data(my_socket, (char*)&size_d, sizeof(size_t));
    if (!size_d && mode == 'd') {
        printf(REMOTE_FILE_ERROR_STR);
        printf(FAILURE_STR);
        return -1;
    }

    // upload/download the file
    FILE* file;
    if (mode == 'd') {
        file = fopen(path, "w+");
        if (write_to_file(my_socket, file, size_d) < 0) {
            printf(FAILURE_STR);
            return -1;
        }
        printf(SUCCESS_STR);
        return 0;
    }
    if (mode == 'u') {
        file = fopen(path, "r");
        if (read_from_file(my_socket, file, size_u) < 0) {
            printf(FAILURE_STR);
            return -1;
        }
        printf(SUCCESS_STR);
        return 0;
    }
    return 0;
}

int run_server(char* path, int port)
{
    // open the socket
    int my_socket, remote_socket;
    if ((my_socket = init_socket(port, true, NULL)) < 0) {
        ERR("error in init_socket()");
        return -1;
    }
    if (listen(my_socket, MAX_QUEUE)) {
        ERR("error in listen()");
        return -1; 
    }

    // set of socket descriptors
    fd_set clientfds;
    fd_set readfds;

    FD_ZERO(&clientfds);
    FD_SET(my_socket, &clientfds);
    FD_SET(STDIN_FILENO, &clientfds);

    while (true)
    {
        /**
         * server's run loop with fds
         */
        readfds = clientfds;

        printf(WAIT_FOR_CLIENT_STR);
        if (select(MAX_CLIENTS+1, &readfds, NULL, NULL, NULL) < 0) {
            ERR("error in select()");
            return -1;
        }
        // incomming connection
        if (FD_ISSET(my_socket, &readfds)) {
            remote_socket = accept(my_socket, NULL, NULL);
            if (remote_socket < 0) {
                ERR("error in accept()");
                return -1;
            }
            FD_SET(remote_socket, &clientfds);
            printf(CONNECTED_SUCCESSFULLY_STR);
        }           
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char in_buf[5];
            scanf("%s", in_buf);
            if (!strcmp(in_buf, "quit")) {
                break;
            }
        } else {
            if (handle_clients(remote_socket, path, port) < 0) {
                printf(FAILURE_STR);
            } else {
                printf(SUCCESS_STR);
            }
            FD_CLR(remote_socket, &clientfds);
        }
    }
    return 0;
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