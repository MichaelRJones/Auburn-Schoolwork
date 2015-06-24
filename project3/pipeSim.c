#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
/*
**		
   PipeSim.c
   COMP 4300
   Michael Jones / Kyle Taylor
   mrj0006	 / atk0011
**
*/


//LATCHES
typedef struct
        { //NOTE: in memory, instruction are store in four separate indexes
		int op_code; // the opcode, and three potential pieces of data
		int off1; // (register1, register2, register3/immediate/address)
		int off2; // each variable in struct corresponds to one.
		int off3; // this could be replaced with a simple variable with an instruction string
        } IF_ID;

typedef struct
        {
		int op_code;
		int rs;
		int rt;
		int rd;
		int op_A;
		int op_B;
		int data;
		int new_PC;
        } ID_Exe;

typedef struct
        {
		int op_code;
		int ALU_out;
		int op_B;
		int rd;
        } Exe_Mem;

typedef struct
        {
		int op_code;
		int MDR;
		int op_B;
		int ALU_out;
		int rd;
        } Mem_WB;

// GLOBAL VARIABLES
int kernel_text[560]; // text section of memory
int kernel_data[140]; // data section of memory
int regs[32]; // registers
int PC = 0x20000500; // Program counter
int CC = 0; // clock cycles
int IC = 0; // instruction count
int NOPS = 0; // number of nops
bool run; // user-mode

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
//
  EXECUTION CYCLE STAGES
//
*/
IF_ID if_stage(int kernel_text[560])
{ // FETCH INSTRUCTION
	IC++;
	IF_ID ifID = {0}; // create latch to return
	ifID.op_code = read(PC - 0x500, kernel_text); // note: instructions are broken up into
	ifID.off1 = read(PC - 0x500 + 1, kernel_text); // opCode, relevant data 1, relevant data 2,
	ifID.off2 = read(PC - 0x500 + 2, kernel_text); // and relevant data 3 (these can be registers or addresses)
	ifID.off3 = read(PC - 0x500 + 3, kernel_text); // So while fetching instruction, I store all four into latch
	PC = PC + 4; // update counter
	return ifID;
}

ID_Exe id_stage(IF_ID ir, Exe_Mem exyOld, Mem_WB memyOld) 
{ // DECODE INSTRUCTION
	ID_Exe idExe = {0}; // create latch to return
	idExe.op_code = ir.op_code; // set the opcode to the previous instructions op code
	if (idExe.op_code == 0x16 || idExe.op_code == 0x27 || idExe.op_code == 0x38 || idExe.op_code == 0x39) {
		idExe.rs = ir.off1; // if BRANCH, need to deal with what instruction is fetched next
		idExe.rt = ir.off2; // set the register source and registers source 2 (because branches can have up to two
		switch (idExe.op_code) { // peices of relevant data)
			case 0x16: // B
				PC = kernel_data[ir.off1 - 0x20000000] + 0x20000500;
				idExe.op_code = 0x99; // sets branch to do nothing in execute (a NOP)
				break;
			case 0x27: // BEQZ
				/* forwarding */
				if (idExe.rs == memyOld.rd) // check if Mem_WB latch updated this register
				{
 	 				idExe.op_A = memyOld.ALU_out; // if so, replace
				}				
				if (idExe.rs == exyOld.rd) // check if Exe_Mem latch updated this register
				{
 	 				idExe.op_A = exyOld.ALU_out; 
				}
				/* execute */
				if (regs[idExe.rs] == 0)
				 {
				 	PC = kernel_data[ir.off2 - 0x20000000] + 0x20000500;
				 }
				idExe.op_code = 0x99; // set to NO-OP (NOP)
				break;
			case 0x38: // BGE
				if (idExe.rs == memyOld.rd) // check if Mem_WB latch updated FIRST register
				{
 	 				idExe.op_A = memyOld.ALU_out; // if so, replace
				}
				if (idExe.rt == memyOld.rd) //check if Mem_WB latch updated SECOND registers
				{ 
					idExe.op_B = memyOld.ALU_out; 
				}				
				if (idExe.rs == exyOld.rd) // check if Exe_Mem latch updated FIRST register
				{
 	 				idExe.op_A = exyOld.ALU_out; 
				}
				if (idExe.rt == exyOld.rd) // check if Exe_Mem latch updated SECOND register
				{ 
					idExe.op_B = exyOld.ALU_out; 
				}
				/* Execute the branch: */
				if (regs[idExe.rs] >= regs[idExe.rt])
				 {
				 	PC = kernel_data[ir.off3 - 0x20000000] + 0x20000500;
				 }
				idExe.op_code = 0x99; // set to do nothing in execute stage
				break;
			case 0x39: // BNE
				idExe.op_code = 0x99; // set to do nothing in execute stage
				/* Forwarding */
				if (idExe.rs == memyOld.rd) // check if Mem_WB latch updated FIRST register 
				{
 	 				idExe.op_A = memyOld.ALU_out; // if so, update
				} 
				if (idExe.rt == memyOld.rd) // check if Mem_WB latch updated SECOND register
				{ 
					idExe.op_B = memyOld.ALU_out; 
				}
				if (idExe.rs == exyOld.rd) // check if Exe_Mem latch updated FIRST register
				{
 	 				idExe.op_A = exyOld.ALU_out; 
				}
				if (idExe.rt == exyOld.rd) // check if Exe_Mem latch updated SECOND register
				{				
					idExe.op_B = exyOld.ALU_out; 
				}
				/* Execute the branch: */
				if (regs[idExe.rs] != regs[idExe.rt])
				 {
				 	PC = kernel_data[ir.off3 - 0x20000000] + 0x20000500;
				 }
				break;		
		}	
	}
	else if (idExe.op_code == 0x0D || idExe.op_code == 0x05 || idExe.op_code == 0x04) { //SUB ADD ADDI
		idExe.rd = ir.off1; // Sub, Add, and Addi have three pieces of info
		idExe.rs = ir.off2; // the register destination (rd), register source 1 (rs)
		idExe.rt = ir.off3; // and register source 2 (rt), so these must be extracted from the
		idExe.op_A = idExe.rs; // input latch. rs and rt can also be called op_A and op_B, so
		idExe.op_B = idExe.rt; // these variables are used for clarity. 
		idExe.data = idExe.rt;	// rt can also be the immediate value or address, so data is used for clarity
	}
	else { // Every other instruction only has two pieces of info
		idExe.rd = ir.off1; // register destination
		idExe.rs = ir.off2; // register source
		idExe.op_A = idExe.rs; // register source can be called op_A for clarification
		idExe.data = idExe.rs; // immediate/address can be data for clarification
	}
	return idExe; // return created latch
}

Exe_Mem exe_stage(ID_Exe idexe, Exe_Mem exyOld, Mem_WB memyOld)
{ // EXECUTE STAGE
	Exe_Mem exemem = {0}; // create latch to return
	exemem.rd = idexe.rd; // store needed info from previous latch to carry on
	exemem.op_B = exemem.op_B;
	exemem.op_code = idexe.op_code;

	/* HAZARD DETECTION (Forwarding) */
	if (memyOld.rd == idexe.rs) // check if Mem_WB updates FIRST register
	{
		regs[idexe.rs] = memyOld.ALU_out; // if so, update
	}
	if (memyOld.rd == idexe.rt) // check if Mem_WB updates SECOND register
	{	
		regs[idexe.rt] = memyOld.ALU_out;
 	}
	if (exyOld.rd == idexe.rs) // check if Exe_Mem updates FIRST register
	{	
		regs[idexe.rs] = exyOld.ALU_out; 
	}
 	if (exyOld.rd == idexe.rt) // check if Exe_Mem updates SECOND register
	{
		regs[idexe.rt] = exyOld.ALU_out;
	}	
	
	/* EXECUTE */
	switch(idexe.op_code)
	{  
	  //ADD Rdest, Rsrc1, Rsrc2
	  case 0x04:
		exemem.ALU_out = regs[idexe.op_A] + regs[idexe.op_B];
		break;
	  //ADDI Rdest, Rsrc1, Imm
	  case 0x05:
		exemem.ALU_out = regs[idexe.op_A] + idexe.data;
	  	break; 
	  //LA Rdest, label
	  case 0x2A:
		exemem.ALU_out = idexe.rs;
		break;
	  //LB Rdest, offset(Rsrc1)
	  case 0x0B:
		exemem.ALU_out = regs[idexe.rs];
		break; 
	  //LI Rdest, Imm
	  case 0x0C:
		exemem.ALU_out = idexe.data;
		break; 
	  //SUBI Rdest, Rsrc1, Imm
	  case 0x0D:
		exemem.ALU_out = regs[idexe.rs] - idexe.data;
		break;
	  // NOP
	  case 0x99:
		NOPS++; // update NOP counter
		break;
	  //SYSCALL
	  case 0x0E:	
		//check if reg2 has changed (since SYSCALL instruction does not include
		if (memyOld.rd == 2) // an rd, the hazard detection was never preformed on
		{		     // the needed register $2 earlier in the function)
			/* forwarding */
			regs[2] = memyOld.ALU_out; 
		}
		if (exyOld.rd == 2) // check if Exe_Mem updated register $2
		{
			/* forwarding */
			regs[2] = exyOld.ALU_out; 
		}
		//
		if (regs[2] == 0x1) // to print string
		{
			//check if reg2 has changed
			if (memyOld.rd == 4) 
			{
				/* forwarding */
				regs[4] = memyOld.ALU_out; 
			}
			if (exyOld.rd == 4) // check if Exe_Mem updated register $2
			{
				/* forwarding */
				regs[4] = exyOld.ALU_out; 
			}
			//
			printf("Answer: %i\n", regs[4]); // print answer
		}
		else if (regs[2] == 0x10) // to exit program
	        {
			exemem.ALU_out = 999999999; // Set ALU_out to some flag number to exit program
		}				    // note: number must never be a potential ALU_out answer
		break;
	  default:
		break;
	}
	return exemem; // return latch
}

Mem_WB mem_stage(Exe_Mem exemem)
{ // MEMORY
	Mem_WB memwb = {0}; // create latch to return
	if (exemem.op_code == 0x0B) // if it was a branch..
	{
		memwb.MDR = read(exemem.ALU_out, kernel_data); // do stuff. for reasons.
	}
	else // otherwise
	{
		memwb.ALU_out = exemem.ALU_out; // pass down the ALU_out
	}
	memwb.op_code = exemem.op_code; // pass down
	memwb.rd = exemem.rd; // " "
	memwb.op_B = exemem.op_B; // " "
	return memwb; // return the latch
}

void wb_stage(Mem_WB memwb, int regs[32])
{
	regs[memwb.rd] = memwb.ALU_out;
	regs[memwb.op_B] = memwb.MDR;
	if (memwb.ALU_out == 999999999)
	{
		printf("# of Clock Cycles: %i\n", CC);
		printf("# of Instructions: %i\n", IC - 5);
		printf("# of NOPS: %i\n", NOPS);
		printf("Exiting Program\n\n");		
		run = false;
	}
}

/*
**
   MAIN FUNCTION
**
*/	

int main()
{
	regs[0] = 0; // set reg 0 to 0 for reasons (apparently this is default)
	loaderModule(kernel_text, kernel_data); // load binary encoded file
	IF_ID IF_ID_new = {0}; 
	IF_ID IF_ID_old = {0}; // create old/new version of latches
	ID_Exe ID_Exe_new = {0};
	ID_Exe ID_Exe_old = {0};
	Exe_Mem Exe_Mem_new = {0};
	Exe_Mem Exe_Mem_old = {0};
	Mem_WB Mem_WB_new = {0};
	Mem_WB Mem_WB_old = {0};
	run = true; // user_mode is on
	while(run) {
		IF_ID_old = IF_ID_new; // update IF_ID
		IF_ID_new = if_stage(kernel_text); // fetch instruction
		ID_Exe_old = ID_Exe_new; // update ID_EXE
		ID_Exe_new = id_stage(IF_ID_old, Exe_Mem_new, Mem_WB_new); // decode instruction
		Exe_Mem_old = Exe_Mem_new; // update Exe_Mem
		Exe_Mem_new = exe_stage(ID_Exe_old, Exe_Mem_old, Mem_WB_new); // execute instruction
		Mem_WB_old = Mem_WB_new; // update Mem_WB
		Mem_WB_new = mem_stage(Exe_Mem_old); // store to memory
		wb_stage(Mem_WB_old, regs); // write to registers
		CC++; // update clock cycle
	}	
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
