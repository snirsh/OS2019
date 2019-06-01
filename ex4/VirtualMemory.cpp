#include <iostream>
#include <unistd.h>
#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#define MSG(msg) std::cout << msg << std::endl;

enum ret_type {EMPTY, MAX, DISTANCE};
struct frame_wrapper {
    word_t index;
    ret_type type;
};

void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

void VMinitialize() {
    MSG("+++++++++++++++++++++++++++++++")
    MSG("+ PAGE_SIZE: " << PAGE_SIZE)
    MSG("+ RAM_SIZE: " << RAM_SIZE)
    MSG("+ VIRTUAL_MEMORY_SIZE: " << VIRTUAL_MEMORY_SIZE)
    MSG("+ NUM_FRAMES: " << NUM_FRAMES)
    MSG("+ NUM_PAGES: " << NUM_PAGES)
    MSG("+++++++++++++++++++++++++++++++")
    MSG("")
    clearTable(0);
}

frame_wrapper rec_helper(word_t index, word_t ignore) {
    usleep(200);
    word_t w, max=0;
    frame_wrapper ret = frame_wrapper({0, EMPTY});
    
    for (word_t i=0; i < PAGE_SIZE; ++i) {
        PMread((index*PAGE_SIZE)+i, &w);
        //MSG("               i="<<i<<"  w="<<w)
        if (w) {
            MSG("               calling rec on "<<w)
            ret = rec_helper(w, ignore);
            if (ret.type == EMPTY) {
                return ret;
            }
        }
        if (w > max) { max = w; }
        if (ret.index > max) { max = ret.index; }
        if (index > max) { max = index; }
    }
    //MSG("               max, index, ignore: "<<max<<" "<<index<<" "<<ignore)
    if (max == 0) {
        if (index == 0) {
            ret = frame_wrapper({1, EMPTY});
            MSG("               rec "<<index<<" return EMPTY "<< 1)
            return ret;
        }
        if (index != ignore) {
            ret = frame_wrapper({index, EMPTY});
            MSG("               rec "<<index<<" return EMPTY "<< index)
            return ret;
        }
    }
    if (index > max) {
        ret = frame_wrapper({index, MAX});
        MSG("               rec "<<index<<" return MAX "<< index)
        return ret; 
    }
    ret = frame_wrapper({max, MAX});
    MSG("               rec "<<index<<" return MAX "<< max)
    return ret;
    // what if page is full? DISTANCE
}

word_t find_frame(word_t ignore) {
    frame_wrapper ret = rec_helper(0, ignore);
    if (ret.type == EMPTY) {
        MSG("               [find_frame] EMPTY: index="<<ret.index)
        return ret.index;
    } else if (ret.type == MAX) {
        if (ret.index == RAM_SIZE-1) {
            // all full
        } else {
        MSG("               [find_frame] MAX: index="<<ret.index)
            return ret.index + 1;
        }
    }
}

int load_page(uint64_t v_addr) {
    MSG("   [load page] vAddr: "<<v_addr)
    MSG("")
    word_t offset, addr1, addr2;
    addr1 = 0;
    for (int i = TABLES_DEPTH ; i >= 0; --i) {
    MSG("       depth: "<<TABLES_DEPTH-i)
        if (i == 0) {
            if (addr1 != addr2) {
                PMrestore(addr1, v_addr/PAGE_SIZE);
            MSG("           [PM restore] put page "<<v_addr/PAGE_SIZE<<" in frame "<<addr1)
            }
            MSG("           page "<<v_addr/PAGE_SIZE<<" already loaded in frame "<<addr1)
            MSG("")
            return addr1;
        }
        offset = (v_addr >> (OFFSET_WIDTH * i)) % PAGE_SIZE;
        PMread(addr1 * PAGE_SIZE + offset, &addr2);
        MSG("           offset: "<<offset)
        if (addr2 == 0) {
            word_t frame = find_frame(addr1);
            MSG("           found frame: "<<frame)
            // make generic
            PMwrite(frame*PAGE_SIZE, 0);
            PMwrite((frame*PAGE_SIZE) + 1, 0);
            PMwrite((addr1 * PAGE_SIZE) + offset, frame);
            addr1 = frame;
        } else {
            addr1 = addr2;
        }
    }
}

int VMread(uint64_t virtualAddress, word_t* value) {
    word_t frame;
    word_t offset = virtualAddress % PAGE_SIZE;
    MSG("[VM read] vAddr: "<<virtualAddress<<"  value: "<<*value)
    MSG("   offset: "<<offset)
    frame = load_page(virtualAddress);
    PMread(frame * PAGE_SIZE + offset, value);
    MSG("   loaded PM frame: "<<frame)
    MSG("   [PM read] addr: "<<frame * PAGE_SIZE + offset<<"  value: "<<*value)
    MSG("")

    return 1;
}

int VMwrite(uint64_t virtualAddress, word_t value) {
    word_t frame;
    word_t offset = virtualAddress % PAGE_SIZE;
    MSG("[VM write] vAddr: "<<virtualAddress<<"  value: "<<value)
    MSG("   offset: "<<offset)
    frame = load_page(virtualAddress);
    PMwrite(frame * PAGE_SIZE + offset, value);
    MSG("   loaded PM frame: "<<frame)
    MSG("   [PM write] addr: "<<frame * PAGE_SIZE + offset<<"  value: "<<value)
    MSG("")

    return 1;
}

void printPM() {
    word_t temp;
    for (int i=0; i<RAM_SIZE; ++i) {
        PMread(i, &temp);
        MSG("["<<i<<"] ["<<temp<<"]")
    }
}


