#include "prints.h"
#include <iostream>
#include <unistd.h>

#define MSG(msg) std::cout << msg << std::endl

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
    switch (mode)
    {
    case 's':
        unsigned short port_num = (unsigned short) strtoul(argv[3], NULL, 0);
        run_server(argv[2], port_num);
        break;
    case 'u': 
        unsigned short port_num = (unsigned short) strtoul(argv[4], NULL, 0);
        run_client_up(argv[2], argv[3], port_num, argv[5]);
        break;
    case 'd':
        unsigned short port_num = (unsigned short) strtoul(argv[4], NULL, 0);
        run_client_down(argv[2], argv[3], port_num, argv[5]);
        break;
    default:
        fprintf(stderr, "Wrong switch inserted, supported switches are -s, -d, -u, you wrote %s", argv[1])
        break;
    }
}

int read_write_data(int s, char *buf, int n, bool read=true){
    /**
     * Reads or writes data from file descriptor to buffer or the opposite 
     * according to number of iterations to read/write which is defined in n.
     * read boolean default is to read (true), if false we write from buf to file.
    **/
    int bcount; /* counts bytes read/written*/
    int br; /* bytes read/written this pass*/
    while(bcount<n){ /* loops until the buffer is full*/
        br = read? (read(s, buf, n-bcount): write(s, buf, n-bcount)); 
        if(br>0){
            bcount += br;
            buf += br;
        }
        if(br<1){
            return(-1);
        }
    }
    return bcount;
}
