#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>

/*
**
    AUTHOR: Michael Jones
		  mrj0006
**
*/

// the answer variable location
#define answer_index 0x200004

/*
**
   WRITE-TO-MEMORY MODULE
**
*/
void write(int data, int addr, int* array)
{
	addr = addr - 0x200000; // get true index
	array[addr] = data;
}

/*
**
   READ-FROM-MEMORY MODULE
**
*/
int read(int addr, int* array)
{	
	addr = addr - 0x200000; //get true index
	return array[addr];
}

/*
**
   ALL POWERFUL LOADER MODULE
**
*/
void loaderModule(int text[280], int data[140])
{
	printf("Enter a file name: "); // get a file name
	char file_name[50];		// large, arbitrary buffer size
	scanf("%s", file_name);

	FILE *ifp = fopen(file_name, "r"); // open file
	if (ifp == NULL)
	{ // if file not found
		perror("Can't open file");
	}
	
	char ch; // the current char in the file
	int textBase = 0x200500; // start of text location in memory
	char opCode[4];
	char addr[8];
	char value[8];
	bool dataMode = false;
	while ( (ch = fgetc(ifp) ) != EOF) // until end of file
	{
		if (ch == '.') // if data or text segment found
		{
			if ( (fgetc(ifp) == 'd') ) // if data segment
			{
				dataMode = true; // signal extracting data
			}
			while ( (ch != '\n') )
			{ // remove the 'text' or 'data' characters from buffer
				ch = fgetc(ifp);
			}
		}
		else if (ch == '\n' || ch == '\0')
		{ // do nothing - crisis averted
		}
		else if (dataMode == false) // for extracting the text part of code
		{	
			// format to hexadecimal
			opCode[0] = ch; 
			addr[0] = ch;
			ch = fgetc(ifp);
			opCode[1] = ch;
			addr[1] = ch;

			// extract opcode
			opCode[2] = fgetc(ifp); // first 4 bits of opcode
			opCode[3] = fgetc(ifp); // last 4 bits of opcode
			
			// extract address
			int j = 2;
			while (j < 8)
			{
				ch = fgetc(ifp);
				addr[j] = ch;
				j++;
			}
			
			// convert to hex from char array
			int opData = convert(opCode);
			int addrData = convert(addr);
	
			// write to memory
			write(opData, textBase - 0x500, text);
			textBase++;
			write(addrData, textBase - 0x500, text);
			textBase++;

		}
		else // for extracting the data part of code
		{
			// format to hexadecimal
			addr[0] = ch; 
			value[0] = ch;
			ch = fgetc(ifp);
			addr[1] = ch;
			value[1] = ch;

			// extract address
			int i = 2;
			while (i < 8)
			{
				ch = fgetc(ifp);
				addr[i] = ch;
				i++;
			}
		
			// extract value
			int k = 2;
			while (k < 8)
			{
				ch = fgetc(ifp);
				value[k] = ch;
				k++;
			}

			// convert to hex from char array
			int trueAddr = convert(addr);
			int valueData = convert(value);
			

			// write to memory
			write(valueData, trueAddr, data);
		}
	}
}

/*
**
   MAIN FUNCTION
**
*/
int main()
{
	int kernel_text[280]; // text section of memory
	int kernel_data[140]; // data section of memory

	loaderModule(kernel_text, kernel_data); // load binary encoded file

	int stack[20]; // stack (max 20)
	int stppt = 0; // stack pointer	

	int PC = 0x200500; // program counter 
	bool user_mode = true; // loop control
	while (user_mode) {
		int opCode = read(PC - 0x500, kernel_text); // get next opcode
		PC++;		
		int addr = read(PC - 0x500, kernel_text); // address for opcode
		switch(opCode)
		{   
			case 0x00:   // PUSH
				stppt++;
				stack[stppt] = read(addr, kernel_data);
				break;
			case 0x01:   // POP
				write(stack[stppt], addr, kernel_data);
				stppt--;
				break;
			case 0x02:   // ADD
				stack[stppt - 1] = stack[stppt] + stack[stppt - 1];
				stppt--;
				break;
			case 0x03:  // MULT
				stack[stppt - 1] = stack[stppt] * stack[stppt - 1];
				stppt--;
				break;
			case 0x04: // END
				user_mode = false;
				int answer = read(answer_index, kernel_data); // obtain answer from
				printf("The answer is: %i\n", answer);
		}
		PC++;
	}	
	return 0;
}	


/*
**
   CONVERT FROM STRING TO HEXADECIMAL - from Stack Overflow
**
*/
int convert(char* string) {
    char *p;
    int uv=0;
    uv=strtoul(string, &p, 16);
    return uv;
}
