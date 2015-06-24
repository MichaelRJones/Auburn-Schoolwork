#include <iostream>
#include <string>
#include <cstdint>
#include <sstream>
#include <fstream>
#include <cstring>
using namespace std;
typedef uint32_t mem_addr;
typedef int32_t mem_word;
typedef int32_t reg_word;


/*
 *  Author: Kyle Taylor
 *  Date:   9/9/14
 */
class Memory {
    public:
	   mem_word read(mem_addr address);
	   void write(mem_addr address, mem_word item);
	   bool load();
	   int convert(char* input);
	   //0 because we are not using an operating system
	   static const mem_addr OP_BASE_ADDR = 0x20000000;
	   static const mem_addr DATA_BASE_ADDR = 0x20000118; //280, no space between op code and data
	   static const mem_addr DATA_OFFSET = 0x00000118;
	   static const mem_addr MEMORY_LEN = 0x000001A4;     //420, the size of our main memory

    private:
	   //40bit memory per instruction, first 8 bits is op code, next 32 bits is data address
	   //140 instructions, 140 places for data values
	   //ex mainMemory[0] = first op code at address 0x20000000, mainMemory[1] is first data address
	   mem_word mainMemory[420];
};


class Accumulator {
    public:
	   int main();

    private:
	   reg_word mRegister;
}; 


//Program Execution
int main() {
    Accumulator accumulator;
    return accumulator.main();
}


//Accumulator switch case
int Accumulator::main() {
    Memory memory;
    bool keepControl = true;
    string anyKey;

    //Make sure the memory loads correctly
    if (!memory.load()) {
	   return 1;
    }
    mem_word instruction;
    mem_addr pc = memory.OP_BASE_ADDR;

    //pc cannot get into the data segment
    while (keepControl && pc < (memory.DATA_BASE_ADDR - 1)) {
	   instruction = memory.read(pc);
	   mem_addr dataAddr = memory.read(pc + 1);
	   switch (instruction) {
		  //Load X
		  case 0x01:
			 mRegister = memory.read(dataAddr);
			 break;
		  //Store X
		  case 0x0A:
			 memory.write(dataAddr, mRegister);
			 break;
		  //Add X
		  case 0x10:
			 mRegister += memory.read(dataAddr);
			 break;
		  //MULT X
		  case 0xA0:
			 mRegister *= memory.read(dataAddr);
			 break;
		  //End
		  case 0xAA:
			 keepControl = false;
			 break;
	   }
	   pc += 2;
    }
    cout << "Register equals: " << mRegister << endl;
    exit(0);	//Problems with return statement
}


//  Convert from string to hexadecimal - from Stack Overflow
int Memory::convert(char* input) {
    char *p;
    int uv=0;
    uv=strtoul(input, &p, 16);
    return uv;
}
//  End code from Stack Overflow


//Load, Read, and Write Memory
mem_word Memory::read(mem_addr address) {
    address -= 0x20000000;
    return mainMemory[address];
}


void Memory::write(mem_addr address, mem_word item) {
    address -= 0x20000000;
    mainMemory[address] = item;
}


bool Memory::load() {
    stringstream ss;
    ifstream fileStream;
    string filename;
    string line;
    int mode;
    
    //Get file name
    cout << "Enter file name: ";
    cin >> filename;
    cout << endl;
    fileStream.open(filename);

    //Make sure file opens correctly
    if (!fileStream.is_open()) {
	   cout << "Error opening file..." << endl << "Ending program";
	   return false;
    }

    mem_addr instructionAddr = OP_BASE_ADDR;
    mem_addr dataAddr;
    mem_word instruction;

    //Read in all file lines
    while (getline(fileStream, line)) {
	   //Keep track of what we are reading in
	   if (line.substr(0, 5) == ".text") {
		  //We are reading in instructions
		  mode = 0;
	   }
	   else if (line.substr(0, 5) == ".data") {
		  //We are reading in data
		  mode = 1;
	   }

	   //TEXT
	   else if (mode == 0) {
		  //make sure it is in the instruction base range
		  if (instructionAddr < (DATA_BASE_ADDR - 1) && instructionAddr >= OP_BASE_ADDR) {
			 char* text;
			 strcpy(text, line.substr(2, 2).c_str());
			 instruction = convert(text);
			 write(instructionAddr, instruction);

			 //Not all text will be followed by a data address
			 if (line.length() > 4) {
				strcpy(text, line.substr(4, 8).c_str());
				dataAddr = convert(text);
				write(instructionAddr + 1, dataAddr + DATA_OFFSET);
			 }
		  }
		  else {
			 cout << "Instruction out of range\n";
		  }
		  instructionAddr += 2;
	   }

	   //DATA
	   else if (mode == 1) {
		  char* text;
		  mem_word data;
		  string debug = line.substr(2, 8);
		  strcpy(text, line.substr(2, 8).c_str());
		  dataAddr = convert(text);
		  dataAddr += DATA_OFFSET;
		  //make sure data is in the data base range
		  if (dataAddr >= DATA_BASE_ADDR && dataAddr < (DATA_BASE_ADDR + MEMORY_LEN)) {
			strcpy(text, line.substr(10, 8).c_str());
			 data = convert(text);
			 write(dataAddr, data);
		  }
		  else {
			 cout << "Instruction out of range\n";
		  }
	   }
    }
    fileStream.close();
    return true;
}
