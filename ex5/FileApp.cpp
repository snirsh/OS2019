#include "prints.h"
#include <iostream>

#define MSG(msg) std::cout << msg << std::endl

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