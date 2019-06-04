#include <iostream>
#include <unistd.h>
#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#define MSG(msg) std::cout << msg << std::endl;
#define ABS(x) (x > 0 ? x : -x)

struct evict_info {
    uint64_t v_addr, distance, link;
    word_t frame;
};

struct tree_node {
    uint64_t depth, req_page;
    word_t frame, ignore, max;
    evict_info ev;
    bool empty;
};

word_t get_distance(word_t page, word_t query)
{
    word_t temp = ABS(page - query);
    return temp < (NUM_PAGES - temp) ? temp : (NUM_PAGES - temp);
}

void clearTable(uint64_t frameIndex)
{
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

void VMinitialize()
{
    MSG("+++++++++++++++++++++++++++++++")
    MSG("+ PAGE_SIZE: " << PAGE_SIZE)
    MSG("+ RAM_SIZE: " << RAM_SIZE)
    MSG("+ VIRTUAL_MEMORY_SIZE: " << VIRTUAL_MEMORY_SIZE)
    MSG("+ NUM_FRAMES: " << NUM_FRAMES)
    MSG("+ NUM_PAGES: " << NUM_PAGES)
    MSG("+ TABLES_DEPTH: " << TABLES_DEPTH)
    MSG("+++++++++++++++++++++++++++++++")
    MSG("")
    clearTable(0);
}

tree_node rec_helper(tree_node node)
{
    word_t w, min_frame, max_index=0;
    evict_info min_ev;
    min_ev.distance = PAGE_SIZE;

    tree_node ret;
    ret.ignore = node.ignore;
    ret.req_page = node.req_page;
    ret.ev.v_addr = node.ev.v_addr;
    ret.max = 0;
    
    for (word_t i=0; i < PAGE_SIZE; ++i) {
        PMread((node.frame * PAGE_SIZE) + i, &w);
        MSG("               i="<<i<<"  w="<<w)
        if (w) {
            if (node.depth < TABLES_DEPTH-1) {
                ret.depth = node.depth + 1;
                ret.ev.v_addr = (node.ev.v_addr << OFFSET_WIDTH) + i;
                ret.frame = w;
                MSG("               calling rec on frame "<<w)
                ret = rec_helper(ret);
                if (ret.empty) {
                    return ret;
                }
            } else {
                ret.ev.v_addr = (node.ev.v_addr << OFFSET_WIDTH) + i;
                ret.ev.distance = get_distance(node.req_page, ret.ev.v_addr);
                ret.ev.frame = w;
                ret.ev.link = (node.frame * PAGE_SIZE) + i;
                MSG("               [LEAF] page: "<<ret.ev.v_addr<<"  frame: "<<w<<"  dist: "<<ret.ev.distance<<"  link: "<<ret.ev.link)
                return ret;
            }
        }
        if (w > max_index) { max_index = w; }
        if (node.frame > max_index) { max_index = node.frame; }
        if (ret.max > max_index) { max_index = ret.max; }

        if (ret.ev.distance < min_ev.distance) {
            min_ev = ret.ev;
        }
    }

    if (max_index == 0) {
        if (ret.frame == 0) {
            ret.frame = 1;
            ret.empty = true;
            MSG("               rec "<<node.frame<<" return EMPTY "<< 1)
            return ret;
        }
        if (ret.frame != ret.ignore) {
            ret.empty = true;
            MSG("               rec "<<node.frame<<" return EMPTY "<< ret.frame)
            return ret;
        }
    }
    ret.ev = min_ev;
    MSG("               rec "<<ret.frame<<" return MAX "<< ret.max)
    return ret;
}

word_t find_frame(uint64_t page_num, word_t ignore)
{
    tree_node node;
    node.frame = 0;
    node.depth = 0;
    node.empty = false;;
    node.req_page = page_num;
    node.ignore = ignore;
    node.ev.v_addr = 0;
    node.ev.distance = PAGE_SIZE;
    node = rec_helper(node);

    if (node.empty) {
        MSG("               [find_frame] EMPTY: frame = "<<node.frame)
        return node.frame;
    } else if (node.max == RAM_SIZE - 1) {
        MSG("               [find_frame] EVICT: frame "<<node.ev.frame<<" to page "<<node.ev.v_addr)
        PMevict(node.ev.frame, node.ev.v_addr);
        clearTable(node.ev.frame);
        PMwrite(node.ev.link, 0);
        return node.ev.frame;
    } else {
        MSG("               [find_frame] MAX: frame = "<<node.frame)
        return node.frame + 1;
    }
}

int load_page(uint64_t v_addr)
{
    MSG("   [load page] vAddr: "<<v_addr)
    MSG("")
    word_t offset, addr1, addr2;
    uint64_t page_num = v_addr/PAGE_SIZE;
    addr1 = 0;

    for (int i = TABLES_DEPTH ; i >= 0; --i) {
    MSG("       depth: "<<TABLES_DEPTH-i)
        if (i == 0) {
            if (addr1 != addr2) {
                PMrestore(addr1, page_num);
            MSG("           [PM restore] put page "<<page_num<<" in frame "<<addr1)
            }
            MSG("           page "<<page_num<<" already loaded in frame "<<addr1)
            MSG("")
            return addr1;
        }
        offset = (v_addr >> (OFFSET_WIDTH * i)) % PAGE_SIZE;
        PMread(addr1 * PAGE_SIZE + offset, &addr2);
        MSG("           offset: "<<offset)
        if (addr2 == 0) {
            word_t frame = find_frame(page_num, addr1);
            clearTable(frame);
            PMwrite((addr1 * PAGE_SIZE) + offset, frame);
            addr1 = frame;
        } else {
            addr1 = addr2;
            MSG("           found ref: "<<addr2)
        }
    }
}

int VMread(uint64_t virtualAddress, word_t* value)
{
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

int VMwrite(uint64_t virtualAddress, word_t value)
{
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

void printPM()
{
    word_t temp;
    for (int i=0; i<RAM_SIZE; ++i) {
        PMread(i, &temp);
        MSG("["<<i<<"] ["<<temp<<"]")
    }
}


