//
// Created by matanos on 6/25/18.
//

#include <random>
#include <iostream>
#include <cassert>
#include "VirtualMemory.h"

#define SUCCESS 1
#define FAIL 0

int main(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) {
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 eng(rd()); // seed the generator
    std::uniform_int_distribution<> distr(0, INT_MAX); // define the range
    std::vector<uint64_t > numbers;
    uint64_t SETS = 5;
    for(uint64_t i = 0 ; i < (SETS * VIRTUAL_MEMORY_SIZE) ; ++i, numbers.push_back((uint64_t)distr(eng)));
    std::cout << "input size: " << numbers.size() << std::endl ;
    std::cout << "TABLE_DEPTH: " << TABLES_DEPTH <<std::endl ;
    std::cout << "NUM_PAGES: " << NUM_PAGES <<std::endl ;
    std::cout << "NUM_FRAMES: " << NUM_FRAMES <<std::endl ;
    std::cout << "PAGE_SIZE: " <<  PAGE_SIZE<<std::endl ;
    std::cout << "VIRTUAL_MEMORY_SIZE: " <<  VIRTUAL_MEMORY_SIZE<<std::endl ;
    std::cout << "RAM_SIZE: " <<  RAM_SIZE<<std::endl ;
    fflush(stdout);
    for(uint64_t j = 0 ; j < SETS ; ++j) {
        VMinitialize();
        for (uint64_t i = (VIRTUAL_MEMORY_SIZE * (j)); i < (VIRTUAL_MEMORY_SIZE * (j + 1)); ++i) {
            std::cerr<<"j is : " <<j << " i is " <<i <<std::endl;
            int result = VMwrite(i % VIRTUAL_MEMORY_SIZE, (word_t) numbers[i]);
            if(result != SUCCESS) {
                std::cerr << "fail at (i = " << i << ",j = " << j << ")" << std::endl;
                std::cerr << "return value was " << result << std::endl;
                assert(false);
            }
        }
        for (uint64_t i = (VIRTUAL_MEMORY_SIZE * (j)); i < (VIRTUAL_MEMORY_SIZE * (j + 1)); ++i) {
            word_t value;
            int result = VMread(i % VIRTUAL_MEMORY_SIZE, &value);
            assert(result == SUCCESS);
            if(result != SUCCESS) {
                std::cerr << "fail at (i = " << i << ",j = " << j << ")" << std::endl;
                std::cerr << "return value was " << result << std::endl;
                assert(false);
            }
            if(uint64_t(value) != (word_t)numbers[i]){
                std::cerr << "fail at (i = " << i << ",j = " << j << ")" << std::endl ;
                std::cerr << "expected: " << (word_t)numbers[i] << ", value is: " << value << std::endl;
                assert(false);
            }
        }
        std::cout << "passed " << j << std::endl;
    }
        VMinitialize();
        for (uint64_t i = 0 ; i < (VIRTUAL_MEMORY_SIZE * SETS); ++i) {
            int result = VMwrite(i % VIRTUAL_MEMORY_SIZE, (word_t) numbers[i]);
            if(result != SUCCESS) {
                std::cerr << "fail at (i = " << i << ")" << std::endl ;
                std::cerr << "return value was " << result << std::endl;
                assert(false);
            }
        }
        for (uint64_t i = (VIRTUAL_MEMORY_SIZE * (SETS -1)); i < (VIRTUAL_MEMORY_SIZE * (SETS)); ++i) {
            word_t value;
            int result = VMread(i % VIRTUAL_MEMORY_SIZE, &value);
            if(result != SUCCESS) {
                std::cerr << "fail at (i = " << i << ")" << std::endl ;
                std::cerr << "return value was " << result << std::endl;
                assert(false);
            }
            if(uint64_t(value) != (word_t)numbers[i]){
                std::cerr << "fail at (i = " << i << ")" << std::endl ;
                std::cerr << "expected: " << (word_t)numbers[i] << ", value is: " << value << std::endl;
                assert(false);
            }
        }
        std::cout << "passed delete test" << std::endl;
    printf("success\n");
}