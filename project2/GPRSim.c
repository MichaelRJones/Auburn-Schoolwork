#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
/*
**		
   GPRSim.c
   COMP 4300
   Michael Jones / Kyle Taylor
   mrj0006	 / atk0011
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
	int newAddr = addr - 0x20000000; // get true index
	array[newAddr] = data;
}

/*
**
   READ-FROM-MEMORY MODULE
**
*/
int read(int addr, int* array)
{	
	addr = addr - 0x20000000; //get true index
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
	int textBase = 0x20000500; // start of text location in memory
	char opCode[4]; // for storing opcodes
	char regi[4];   // for storing any registers
	char addr[10];	// for storing any addresses
	char value[8];	// for storing data values
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
		else if (ch == '\n' || ch == '\0' || ch == '\r')
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

			if (opCode[2] == 'F') { //label
				// extract register
				int j = 0;
				while (j < 4)
				{
					ch = fgetc(ifp);
					regi[j] = ch;
					j++;
				}

				// extract address
				j = 2;
				while (j < 10)
				{
					ch = fgetc(ifp);
					addr[j] = ch;
					j++;
				}
			
				// convert to hex from char array
				
				int addrData = convert(addr);
				int regData = convert(regi);
				write(regData, addrData, data);
			}
			
			else if (opCode[2] == '1') // B
			{
				// extract address
				int j = 2;
				while (j < 10)
				{
					ch = fgetc(ifp);
					addr[j] = ch;
					j++;
				}
				j = 0;
				while (j < 4)
				{
					ch = fgetc(ifp);
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
				write(0, textBase - 0x500, text);
				textBase++;
				write(0, textBase - 0x500, text);
				textBase++;
	}
			else if (opCode[2] == '2') //BEQZ, LA
			{
				// extract register
				int j = 0;
				while (j < 4)
				{
					ch = fgetc(ifp);
					regi[j] = ch;
					j++;
				}

				// extract address
				j = 2;
				while (j < 10)
				{
					ch = fgetc(ifp);
					addr[j] = ch;
					j++;
				}
			
				// convert to hex from char array
				int opData = convert(opCode);
				int addrData = convert(addr);
				int regData = convert(regi); 				

				// write to memory
				write(opData, textBase - 0x500, text);
				textBase++;
				write(regData, textBase - 0x500, text);
				textBase++;
				write(addrData, textBase - 0x500, text); 
				textBase++;
				
				write(0, textBase - 0x500, text);
				textBase++;
			}

			else if (opCode[2] == '3') //BGE, BNE
			{
				int opData = convert(opCode);
				write(opData, textBase - 0x500, text);
				textBase++;
				// extract register
				int j = 0;
				while (j < 4)
				{
					ch = fgetc(ifp);
					regi[j] = ch;
					j++;
				}

				int regData = convert(regi);
				write(regData, textBase - 0x500, text);
				textBase++;
				// extract register
				j = 0;
				while (j < 4)
				{
					ch = fgetc(ifp);
					regi[j] = ch;
					j++;
				}
				
				// extract address
				j = 2;
				while (j < 10)
				{
					ch = fgetc(ifp);
					addr[j] = ch;
					j++;
				}
			
				// convert to hex from char array
				int addrData = convert(addr);
				regData = convert(regi); 				

				// write to memory
				write(regData, textBase - 0x500, text);
				textBase++;
				write(addrData, textBase - 0x500, text);
				textBase++;
			}
			else { // every other instruction
				int opData = convert(opCode);
				write(opData, textBase - 0x500, text);
				textBase++;
				
				// extract register
				int j = 0;
				while (j < 4)
				{
					ch = fgetc(ifp);
					regi[j] = ch;
					j++;
				}

				int regData = convert(regi);
				
				write(regData, textBase - 0x500, text);
				textBase++;

				// extract register
				j = 0;
				while (j < 4)
				{
					ch = fgetc(ifp);
					regi[j] = ch;
					j++;
				}

				regData = convert(regi);
				write(regData, textBase - 0x500, text);
				textBase++;

				// extract register
				j = 0;
				while (j < 4)
				{
					ch = fgetc(ifp);
					regi[j] = ch;
					j++;
				}

				regData = convert(regi);
				write(regData, textBase - 0x500, text);
				textBase++;
			}

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
			while (i < 10)
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
	int kernel_text[560]; // text section of memory
	int kernel_data[140]; // data section of memory

	loaderModule(kernel_text, kernel_data); // load binary encoded file
	int regs[32]; // registers	
	char realText[1024];
	int PC = 0x20000500; // program counter
	int IC = 0; // instruction count
	int C = 0; //  total cycles of execution
	int speedup = 0; // speedup (uncalculated)
	bool user_mode = true; // loop control
	while (user_mode) {
		int opCode = read(PC - 0x500, kernel_text); // get next opcode
		PC++;		
		int reg1 = read(PC - 0x500, kernel_text); // first component
		PC++;
		int reg2 = read(PC - 0x500, kernel_text); // second component
		PC++;		
		int reg3 = read(PC - 0x500, kernel_text); // final component
		PC++;
		switch(opCode)
		{   
		  //ADDI Rdest, Rsrc1, Imm
		  case 0x05:
			 regs[reg1] = regs[reg2] + reg3;
			 C = C + 6;	 
		  break;
		  //B label
		  case 0x16:
			 PC = kernel_data[reg1 - 0x20000000] + 0x20000500;
			 C = C + 4;
			 break;
	      	  // BEQZ Rsrc1, label
		  case 0x27:
			 if (regs[reg1] == 0)
			 {
			 	PC = kernel_data[reg2 - 0x20000000] + 0x20000500;
			 }
			 C = C + 5;
			 break; 
		  //BGE Rsrc1, Rsrc2, label
		  case 0x38:
			 if (regs[reg1] >= regs[reg2])
			 {
			 	PC = kernel_data[reg3 - 0x20000000] + 0x20000500;
			 }
			C = C + 5;
			 break; 
		  //BNE Rsrc1, Rsrc2, label
		  case 0x39:
			 if (regs[reg1] != regs[reg2])
			 {
			 	PC = kernel_data[reg3 - 0x20000000] + 0x20000500;
			 }
			C = C + 5;
			 break; 
		  //LA Rdest, label
		  case 0x2A:
			 regs[reg1] = reg2;
			C = C + 5;
			 break;
		  //LB Rdest, offset(Rsrc1)
		  case 0x0B:
			regs[reg1] = read(regs[reg2], kernel_data);
			C = C + 6;
			break; 
		  //LI Rdest, Imm
		  case 0x0C:
			 regs[reg1] = reg2;
			C = C + 3;
			 break; 
		  //SUBI Rdest, Rsrc1, Imm
		  case 0x0D:
			regs[reg1] = regs[reg2] - reg3;
			C = C + 6;
			break; 
		  //SYSCALL
		  case 0x0E:
			  if (regs[4] == 4) // to print string
			  {
				  if (read(regs[5], kernel_data) == 0) {
				  	printf("The string is not a palindrome.\n");
				  }
				  else {
			 	  	printf("The string is a palindrome.\n");
				  }
			  }
			  else if (regs[4] == 8) // to read string
			  {
				   printf("Enter a word: ");
    				   scanf("%s", realText);
				   int i = 0;
				   char c = realText[i];
				   int addr = 0x20000009;
				   while (c != '\0')
				   {
					write(c, addr, kernel_data);
					addr++;
				   	i++;
					c = realText[i];
				   }
				   write(0, addr, kernel_data);
			  }
			  else if (regs[4] == 10) // to exit program
			  {
				  printf("IC: %i\nC: %i\nSpeedup: %f\n", IC,C,(double)(8*IC)/C);
				  user_mode = false;
			  }
			 C = C + 8;
			 break;
		}
		IC++;
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
