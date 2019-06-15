//
// Created by evyatar100 on 5/27/19.
// ver 3.0
//

#include <cstdio>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <vector>
#include <iostream>

#include "VirtualMemory.h"
#include "PhysicalMemory.h"

using namespace std;

/**
 * fill the physical memory with zeros
 */
void resetRAM();

/**
 * validate that the physical memory is equal to expectedRAM. if it does not, halt's the script
 */
void assertRAM(std::vector<word_t> expectedRAM);

/**
 * print's the virtual memory tree. feel free to use this function is Virtual Memory for debuging.
 */
void printTree();

/**
 * print's the virtual memory tree. feel free to use this function is Virtual Memory for debuging.
 */
void printRAM();

/**
 * get a copy of the current physical memory
 */
std::vector<word_t> getRam();

void printRAMSimple();

std::vector<word_t> state0 = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
std::vector<word_t> state1 = {1, 0, 0, 2, 0, 3, 4, 0, 0, 3, 0, 0, 0, 0, 0, 0};
std::vector<word_t> state2 = {1, 0, 5, 2, 0, 3, 4, 0, 0, 3, 0, 6, 0, 7, 0, 0};
std::vector<word_t> state3 = {1, 4, 5, 0, 0, 7, 0, 2, 0, 3, 0, 6, 0, 0, 0, 0};
std::vector<word_t> state4 = {1, 4, 0, 6, 0, 0, 0, 2, 0, 3, 7, 0, 0, 5, 0, 3};
std::vector<word_t> state5 = {1, 4, 0, 6, 0, 7, 0, 2, 0, 3, 0, 0, 0, 5, 0, 99};

static bool printType = false; // You may change this to True if you want.

int main(int argc, char **argv)
{
	if (OFFSET_WIDTH != 1 or PAGE_SIZE != 2 or RAM_SIZE != 16 or VIRTUAL_ADDRESS_WIDTH != 5 or
		VIRTUAL_MEMORY_SIZE != 32)
	{
		std::cout <<
			 "please override MemoryConstanrs.h with the version provided with the tester.\n";
		exit(2);
	}

	cout <<
		 "Virtual Memory size is 32 = 2^5\n"
		 "Page/Frame/Table size is 2 = 2^1\n"
		 "There are 4 layers is the tree\n"
		 "Ram size is 16\n\n" << endl;

	word_t word;
	resetRAM();
	VMinitialize();

	printRAM();
	printTree();
	cout <<
		 "State 0, after calling:\nVMwrite(13, 3)" << endl;
	assertRAM(state0);

	VMwrite(13, 3);
	printRAM();
	printTree();
	cout <<
		 "State 1, after calling:\nVMwrite(13, 3)" << endl;
	assertRAM(state1);


	VMread(13, &word);
	printRAM();
	printTree();
	cout <<
		 "State 1a, after calling:\nVMwrite(13, 3)\nVMread(13, &word)" << endl;
	assertRAM(state1);
	if (word != 3)
	{
		cout
				<< "After the call VMwrite(13, 3) and then VMread(13, &word), word should be equal to 3 but"
				   " word = " << word << endl;
		cout << "Test failed. Aborting..." << endl;
		exit(1);
	}

	VMread(6, &word);
	printRAM();
	printTree();
	cout <<
		 "State 2, after calling:\nVMwrite(13, 3)\nVMread(13, &word)\nVMread(6, &word)" << endl;
	assertRAM(state2);


	VMread(31, &word);
	printRAM();
	printTree();
	cout <<
		 "State 3, after calling:\nVMwrite(13, 3)\nVMread(13, &word)\nVMread(6, &word)\nVMread(31, &word)"
		 << endl;
	assertRAM(state3);

	VMread(13, &word);
	printRAM();
	printTree();
	cout <<
		 "State 4, after calling:\nVMwrite(13, 3)\nVMread(13, &word)\nVMread(6, &word)\nVMread(31, &word)\nVMread(13, &word)"
		 << endl;
	assertRAM(state4);

	if (word != 3)
	{
		cout
				<< "At this point, word should be equal to 3 but"
				   " word = " << word << endl;
		cout << "Test failed. Aborting..." << endl;
		exit(1);
	}


	VMwrite(31, 99);
	printRAM();
	printTree();
	cout <<
		 "State 5, after calling:\nVMwrite(13, 3)\nVMread(13, &word)\nVMread(6, &word)\nVMread(31, &word)\nVMread(13, &word)\nVMwrite(31, 99)"
		 << endl;
	assertRAM(state5);

	cout << "\n\n$$$ Test passed $$$" << endl;
	return 0;
}


// ---------------------------------------- boring stuff ---------------------------------------------------
/**
 * fill the physical memory with zeros
 */
void resetRAM()
{
	std::vector<word_t> RAM(NUM_FRAMES * PAGE_SIZE);
	for (uint64_t i = 0; i < RAM_SIZE; i++)
	{
		PMwrite(i, 0);
	}
}


/**
 * validate that the physical memory is equal to expectedRAM. if it does not, halt's the script
 */
void assertRAM(std::vector<word_t> expectedRAM)
{
	std::vector<word_t> actualRAM = getRam();
	for (uint64_t i = 0; i < RAM_SIZE; i++)
	{
		if (actualRAM[i] != expectedRAM[i])
		{
			cout << "Your physical memory is not in its required state. See FlowExample.pdf" << endl;
			cout << "Test failed. Aborting..." << endl;
			exit(1);
		}
	}
	cout << "Your physical memory state is correct." << endl;
}

/*
 * helper for printTree
 */
void printSubTree(uint64_t root, int depth, bool isEmptyMode)
{
	if (depth == TABLES_DEPTH)
	{
		return;
	}
	word_t currValue = 0;

	if ((isEmptyMode || root == 0) && depth != 0)
	{
		isEmptyMode = true;
	}

	//right son
	PMread(root * PAGE_SIZE + 1, &currValue);
	printSubTree(static_cast<uint64_t>(currValue), depth + 1, isEmptyMode);

	//father
	for (int _ = 0; _ < depth; _++)
	{
		std::cout << '\t';
	}
	if (isEmptyMode)
	{
		std::cout << '_' << '\n';
	} else
	{
		if (depth == TABLES_DEPTH - 1)
		{
			word_t a, b;
			PMread(root * PAGE_SIZE + 0, &a);
			PMread(root * PAGE_SIZE + 1, &b);
			std::cout << root << " -> (" << a << ',' << b << ")\n";
		} else
		{
			std::cout << root << '\n';
		}
	}

	//left son
	PMread(root
		   * PAGE_SIZE + 0, &currValue);
	printSubTree(static_cast <uint64_t>(currValue), depth + 1, isEmptyMode);
}

/**
 * print's the virtual memory tree. feel free to use this function is Virtual Memory for debuging.
 */
void printTree()
{
	std::cout << "---------------------" << '\n';
	std::cout << "Virtual Memory:" << '\n';
	printSubTree(0, 0, false);
	std::cout << "---------------------" << '\n';
}

/**
 * get a copy of the current physical memory
 */
std::vector<word_t> getRam()
{
	std::vector<word_t> RAM(RAM_SIZE);
	word_t tempWord;
	for (uint64_t i = 0; i < NUM_FRAMES; i++)
	{
		for (uint64_t j = 0; j < PAGE_SIZE; j++)
		{
			PMread(i * PAGE_SIZE + j, &tempWord);
			RAM[i * PAGE_SIZE + j] = tempWord;
		}
	}
	return RAM;
}


/**
 * print the current state of the pysical memory. feel free to use this function is Virtual Memory for debuging.
 */
void printRAM()
{
	std::cout << "---------------------" << '\n';
	std::cout << "Physical Memory:" << '\n';
	std::vector<word_t> RAM = getRam();

	if (printType)
	{
		for (uint64_t i = 0; i < NUM_FRAMES; i++)
		{
			std::cout << "frame " << i << ":\n";
			for (uint64_t j = 0; j < PAGE_SIZE; j++)
			{
				std::cout << "(" << j << ") " << RAM[i * PAGE_SIZE + j] << "\n";
			}
			std::cout << "-----------" << "\n";
		}
	} else
	{

		std::cout << "FRAME INDICES -\t";
		for (uint64_t i = 0; i < NUM_FRAMES; i++)
		{
			std::cout << "F" << i << ": (";
			for (uint64_t j = 0; j < PAGE_SIZE - 1; j++)
			{
				std::cout << j << ",\t";
			}
			std::cout << PAGE_SIZE - 1 << ")\t";
		}
		std::cout << '\n';
		std::cout << "DATA -\t\t\t";
		for (uint64_t i = 0; i < NUM_FRAMES; i++)
		{
			std::cout << "     ";
			for (uint64_t j = 0; j < PAGE_SIZE - 1; j++)
			{
				std::cout << RAM[i * PAGE_SIZE + j] << ",\t";
			}
			std::cout << RAM[i * PAGE_SIZE + PAGE_SIZE - 1] << " \t";
		}
		std::cout << '\n';
	}

	std::cout << "---------------------" << '\n';
}


/*
 * print  the current state of the pysical memory.
 * the print will look like this { 1, 4, 0, 7, 0, 5, 0, 2, 0, 3, 0, 99, 0, 0, 0, 6}. to create state.
 */
void printRAMSimple()
{
	std::vector<word_t> RAM = getRam();
	std::cout << "{ ";
	for (uint64_t i = 0; i < RAM_SIZE - 1; i++)
	{
		std::cout << RAM[i] << ", ";
	}
	std::cout << RAM[RAM.size() - 1] << "}" << "\n";
}
