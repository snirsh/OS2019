#include "VirtualMemory.h"
#include "PhysicalMemory.h"

void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

void VMinitialize() {
    clearTable(0);
}


int VMread(uint64_t virtualAddress, word_t* value) {
    uint64_t seg, addr1;
    word_t addr2;
    addr1 = 0;
    for (uint64_t i = TABLES_DEPTH ; i >= 0; i--) {
        seg = (virtualAddress >> (OFFSET_WIDTH * i)) && ((uint64_t)(PAGE_SIZE-1));
        if (i == 0) {
            PMread(addr1 * PAGE_SIZE + seg, value);
            return 1;
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
    return 0;
}


int VMwrite(uint64_t virtualAddress, word_t value) {

    uint64_t seg, addr1;
    word_t addr2;
    addr1 = 0;
    for (uint64_t i = TABLES_DEPTH ; i >= 0; i--) {
        seg = (virtualAddress >> (OFFSET_WIDTH * i)) && ((uint64_t)(PAGE_SIZE-1));
        if (i == 0) {
            PMwrite(addr1 * PAGE_SIZE + seg, value);
            return 1;
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
    return 0;
}

uint64_t find_frame(uint64_t index, uint64_t ignore) {
    uint64_t ret = rec_helper(index, ignore);
    if (ret % 2 == 0) {
        return ret >> 1;
    } else {
        return (ret >> 1) + 1;
    }
}

uint64_t rec_helper(uint64_t index, uint64_t ignore) {

    word_t w1, w2;
    uint64_t ret1, ret2;

    PMread(index << 1, &w1);
    PMread((index << 1) + 1, &w2);
    if (!(w1 && w2) && index != ignore) {
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
