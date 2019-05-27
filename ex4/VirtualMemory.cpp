#include <iostream>
#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#define MSG(msg) std::cout << msg << std::endl;

void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

void VMinitialize() {
    MSG("PAGE_SIZE: " << PAGE_SIZE)
    MSG("RAM_SIZE: " << RAM_SIZE)
    MSG("VIRTUAL_MEMORY_SIZE: " << VIRTUAL_MEMORY_SIZE)
    clearTable(0);
}

uint64_t rec_helper(uint64_t index, uint64_t ignore) {

    word_t w1, w2;
    uint64_t ret1, ret2;

    PMread(index << 1, &w1);
    PMread((index << 1) + 1, &w2);
    if (!((w1 && w2) && (index != ignore))) {
        return index << 1;
    }

    ret1 = rec_helper(w1, ignore);
    ret2 = rec_helper(w2, ignore);

    if (ret1 % 2 == 0) {
        return ret1;
    } else if (ret2 % 2 == 0) {
        return ret2;
    } else {
        return ret1 > ret2 ? (ret1 << 1) + 1  : (ret2 << 1) + 1;
    }
}

uint64_t find_frame(uint64_t index, uint64_t ignore) {
    uint64_t ret = rec_helper(index, ignore);
    if (ret % 2 == 0) {
        return ret >> 1;
    } else {
        return (ret >> 1) + 1;
    }
}

int load_page(uint64_t virtualAddress) {
    uint64_t seg, addr1;
    word_t addr2;
    addr1 = 0;
    for (uint64_t i = TABLES_DEPTH ; i >= 0; --i) {
        seg = (virtualAddress >> (OFFSET_WIDTH * i)) && ((uint64_t)(PAGE_SIZE-1));
        if (i == 0) {
            if (addr1 != addr2) {
                PMrestore(addr1, virtualAddress);
            }
            return addr1;
        }
        PMread(addr1 * PAGE_SIZE + seg, &addr2);
        if (addr2 == 0) {
            uint64_t frame = find_frame(0, addr1);
            PMwrite(frame << 1, 0);
            PMwrite((frame << 1)+1, 0);
            PMwrite(addr1 * PAGE_SIZE + seg, frame);
            addr1 = frame;
        } else {
            addr1 = addr2;
        }
    }
}

int VMread(uint64_t virtualAddress, word_t* value) {
    word_t frame;
    uint64_t offset;
    offset = virtualAddress && ((uint64_t)(PAGE_SIZE-1));
    frame = load_page(virtualAddress);
    PMread(frame * PAGE_SIZE + offset, value);
    return 1;
}

int VMwrite(uint64_t virtualAddress, word_t value) {
    word_t frame;
    uint64_t offset;
    offset = virtualAddress && ((uint64_t)(PAGE_SIZE-1));
    frame = load_page(virtualAddress);
    PMwrite(frame * PAGE_SIZE + offset, value);
    return 1;
}


