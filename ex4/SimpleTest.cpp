#include "VirtualMemory.h"
#include <iostream>
#include <cstdio>
#include <cassert>

#define MSG(msg) std::cout << msg << std::endl;

int main(int argc, char **argv) {

    word_t value;
    VMinitialize();
    VMwrite(13, 3);
    printPM();
    VMread(6, &value);
    printPM();
    VMread(31, &value);


    /*
    VMinitialize();
    for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i) {
        printf("writing to %llu\n", (long long int) i);
        VMwrite(5 * i * PAGE_SIZE, i);
    }

    for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i) {
        word_t value;
        VMread(5 * i * PAGE_SIZE, &value);
        printf("reading from %llu %d\n", (long long int) i, value);
        assert(uint64_t(value) == i);
    }
    printf("success\n");
    */
    return 0;
}