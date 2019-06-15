#include "VirtualMemory.h"
#include <iostream>
#include <cstdio>
#include <cassert>

#define MSG(msg) std::cout << msg << std::endl;

int main(int argc, char **argv) {

    VMwrite(5 * 0 * PAGE_SIZE, 0);
    VMwrite(5 * 1 * PAGE_SIZE, 1);
    VMwrite(5 * 2 * PAGE_SIZE, 2);
    VMwrite(5 * 3 * PAGE_SIZE, 3);
    VMwrite(5 * 4 * PAGE_SIZE, 4);

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