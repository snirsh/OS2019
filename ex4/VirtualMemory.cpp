#include <iostream>
#include <unistd.h>
#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#define MSG(msg) std::cout << msg << std::endl;
#define ABS(x) (x > 0 ? x : -x)
std::string indent = "            ";

struct tree_node {
    uint64_t depth, req_page, ev_addr, ev_distance;
    word_t frame, ignore, max, ev_frame, link;
    bool empty, link_toggle;
};

uint64_t get_distance(uint64_t page, uint64_t query)
{
    uint64_t temp = ABS(page - query);
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
    indent += "    ";
    word_t w, max_index = 0;
    uint64_t max_distance = 0;
    uint64_t ev_addr;
    word_t ev_frame, ev_link;

    tree_node ret;
    ret.ignore = node.ignore;
    ret.req_page = node.req_page;
    ret.ev_addr = node.ev_addr;
    ret.frame = node.frame;
    ret.empty = false;
    ret.link_toggle = false;
    ret.link = 0;
    ret.max = 0;
    ret.ev_distance = 0;

    for (word_t i=0; i < PAGE_SIZE; ++i) {
        PMread((node.frame * PAGE_SIZE) + i, &w);
        MSG(indent<<"i="<<i<<"  w="<<w)
        if (w) {
            if (node.depth < TABLES_DEPTH-1) {
                ret.depth = node.depth + 1;
                ret.ev_addr = (node.ev_addr << OFFSET_WIDTH) + i;
                ret.frame = w;
                MSG(indent<<"calling rec on frame "<<w)
                ret = rec_helper(ret);
                if (ret.empty) {
                    if (ret.link_toggle) {
                        ret.link = (node.frame * PAGE_SIZE) + i;
                        ret.link_toggle = false;
                    }
                    indent = indent.substr(0,indent.length() - 4);
                    return ret;
                }
            } else {
                ret.ev_addr = (node.ev_addr << OFFSET_WIDTH) + i;
                ret.ev_distance = get_distance(node.req_page, ret.ev_addr);
                ret.ev_frame = w;
                ret.link = (node.frame * PAGE_SIZE) + i;
                MSG(indent<<"[LEAF] page: "<<ret.ev_addr<<"  frame: "<<w<<"  dist: "<<ret.ev_distance<<"  link: "<<ret.link)
            }
        }
        if (w > max_index) { max_index = w; }
        if (ret.max > max_index) { max_index = ret.max; }

        if (ret.ev_distance > max_distance) {
            MSG(indent<<"new max leaf. frame: "<<ret.ev_frame<<" distance: "<< ret.ev_distance)
            max_distance = ret.ev_distance;
            ev_frame = ret.ev_frame;
            ev_link = ret.link;
            ev_addr = ret.ev_addr;
        }
    }

    if (max_index == 0) {
        if ((ret.frame != ret.ignore) || (ret.frame == 0)) {
            ret.empty = true;
            ret.link_toggle = true;
            MSG(indent<<"rec "<<node.frame<<" return EMPTY "<< ret.frame)
            indent = indent.substr(0,indent.length() - 4);
            return ret;
        }
    }
    ret.max = max_index;
    ret.ev_distance = max_distance;
    ret.ev_frame = ev_frame;
    ret.ev_addr = ev_addr;
    ret.link = ev_link;
    MSG(indent<<"rec "<<ret.frame<<" return MAX "<< ret.max)
    indent = indent.substr(0,indent.length() - 4);
    return ret;
}

word_t find_frame(uint64_t page_num, word_t ignore)
{
    tree_node node;
    node.frame = 0;
    node.depth = 0;
    node.empty = false;
    node.req_page = page_num;
    node.ignore = ignore;
    node.ev_addr = 0;
    node.ev_distance = 0;
    node = rec_helper(node);

    if (node.empty) {
        MSG("           [find_frame] EMPTY: frame = "<<node.frame)
        if (node.frame == 0) {return 1;}
        PMwrite(node.link, 0);
        return node.frame;
    } else if (node.max < NUM_FRAMES - 1) {
        MSG("           [find_frame] MAX: frame = "<<node.max + 1)
        return node.max + 1;
    } else {
        MSG("           [find_frame] EVICT: frame "<<node.ev_frame<<" to page "<<node.ev_addr)
        PMevict(node.ev_frame, node.ev_addr);
        clearTable(node.ev_frame);
        PMwrite(node.link, 0);
        return node.ev_frame;
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
            MSG("       [PM restore] put page "<<page_num<<" in frame "<<addr1)
            }
            MSG("       page "<<page_num<<" already loaded in frame "<<addr1)
            MSG("")
            return addr1;
        }
        offset = (v_addr >> (OFFSET_WIDTH * i)) % PAGE_SIZE;
        PMread(addr1 * PAGE_SIZE + offset, &addr2);
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


