#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
/*
**		
   scoSim.c
   COMP 4300
   Michael Jones / Kyle Taylor
   mrj0006	 / atk0011
**
*/

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


//LATCHES
typedef struct
        { //NOTE: in memory, instruction are store in four separate indexes
		int op_code; // the opcode, and three potential pieces of data
		float off1; // (register1, register2, register3/immediate/address)
		float off2; // each variable in struct corresponds to one.
		float off3; // this could be replaced with a simple variable with an instruction string
        } IF_ID;

typedef struct
        {
		int op_code;
		float rs;
		float rt;
		float rd;
		float op_A;
		float op_B;
		float data;
		int new_PC;
		int pipeline;
        } ID_Exe;

typedef struct
        {
		int op_code;
		float ALU_out;
		float op_B;
		float rd;
        } Exe_Mem;

typedef struct
        {
		int op_code;
		int MDR;
		float op_B;
		float ALU_out;
		float rd;
        } Mem_WB;
/*
**
   SCOREBOARD
**
*/
enum funcU {null, Integer, Mult1, Mult2, Add, Divide };
enum instrStatus {no, Issue, Read, Execution, Write };

struct fUnitStatus {
  	int busy; // boolean
  	int op; //operation to perform
  	int fi; //destination
  	int fj; //source 1
  	int fk; //source 2
  	enum funcU qj;  //producing fj
  	enum funcU qk;  //producing fk
  	int rj; //boolean fj ready
  	int rk; //boolean fk ready
};

struct registerResultStatus {
  	enum funcU fpStatus[30]; //floating point register status
  	enum funcU status[32];   //regular register status
};
struct Scoreboard {
	enum instrStatus status[5]; //latest instruction in the corresponding FU
  	struct fUnitStatus integer;
  	struct fUnitStatus fpMult1;
  	struct fUnitStatus fpMult2;
  	struct fUnitStatus fpAdd;
  	struct fUnitStatus fpDivide;
  	struct registerResultStatus regResultStatus;
};


// GLOBAL VARIABLES
int kernel_text[560]; // text section of memory
int kernel_data[140]; // data section of memory
int regs[32]; // registers
float fregs[32]; // registers
int PC = 0x20000500; // Program counter
int CC = 0; // clock cycles
int IC = 0; // instruction count
int NOPS = 0; // number of nops
bool run; // user-mode
int sco;
struct Scoreboard scoreboard;
/*
**
======= PIPELINES
**
*/
// Integer Pipeline
Exe_Mem updateIntPipeline1() {
	ID_Exe idexe = {0};
	Exe_Mem exemem = {0}; // create latch to return
	if (scoreboard.integer.rj && scoreboard.integer.rk && scoreboard.status[0] == Issue)
	{
		idexe.op_code = scoreboard.integer.op;
		idexe.rd = scoreboard.integer.fi;
		idexe.rs = scoreboard.integer.fj;
		idexe.rt = scoreboard.integer.fk;
		idexe.op_A = idexe.rs;
		idexe.op_B = idexe.rt;
		idexe.data = idexe.rs;
		scoreboard.status[0] = Execution;
	}
	if (scoreboard.status[0] != Execution) {
		return exemem;
	}
	exemem.rd = idexe.rd; // store needed info from previous latch to carry on
	exemem.op_B = idexe.op_B;
	exemem.op_code = idexe.op_code;
	/* EXECUTE */
	switch(idexe.op_code)
	{  
	  case 0x0F: // LD
		fregs[(int) idexe.rd] = idexe.data;
		break;
	  case 0x38: // BGE
		if (fregs[(int) idexe.rs] >= fregs[(int) idexe.rt]) {
			PC = kernel_data[1] + 0x20000500;
		}
	  	break;
	  //ADD Rdest, Rsrc1, Rsrc2
	  case 0x04:
		exemem.ALU_out = regs[(int) idexe.op_A] + regs[(int) idexe.op_B];
		break;
	  //ADDI Rdest, Rsrc1, Imm
	  case 0x05:
		exemem.ALU_out = regs[(int) idexe.op_A] + idexe.data;
	  	break; 
	  //LA Rdest, label
	  case 0x2A:
		exemem.ALU_out = idexe.rs;
		break;
	  //LB Rdest, offset(Rsrc1)
	  case 0x0B:
		exemem.ALU_out = regs[(int) idexe.rs];
		break; 
	  //LI Rdest, Imm
	  case 0x0C:
		exemem.ALU_out = idexe.data;
		break; 
	  //SUBI Rdest, Rsrc1, Imm
	  case 0x0D:
		fregs[9] = fregs[9] - 1;
		break;
	  // NOP
	  case 0x99:
		NOPS++; // update NOP counter
		break;
	  //SYSCALL
	  case 0x0E:	
		if (fregs[9] == 0x00) // to exit program
	        {
			printf("X0 = %f\n", fregs[7]); // Display answer
			printf("Y0 = %f\n\n", fregs[8]);
			printf("CC = %i\nIC = %i\nNOPs = %i\n", CC, IC, NOPS);
			run = false;
		}				    
		break;
	  default:
		break;
	}
	scoreboard.status[0] = Write;
	return exemem;
}

// MEM stage
void updateIntPipeline2(Mem_WB wbArray[5], Exe_Mem lat)
{
	Mem_WB memwb = {0}; // create latch to return
	if (lat.op_code == 0x0B) // if it was a branch..
	{
		//memwb.MDR = read(lat.ALU_out, kernel_data); // do stuff. for reasons.
	}
	else // otherwise
	{
		memwb.ALU_out = lat.ALU_out; // pass down the ALU_out
	}
	memwb.op_code = lat.op_code; // pass down
	memwb.rd = lat.rd; // " "
	memwb.op_B = lat.op_B; // " "
	wbArray[0] = memwb; // return the latch
}

// Multiply 1 Pipeline
void updateMult1Pipeline(Mem_WB wbArray[5]) {
	ID_Exe idexe = {0};
	if (scoreboard.fpMult1.rj && scoreboard.fpMult1.rk && scoreboard.status[1] == Issue)
	{
		idexe.op_code = scoreboard.fpMult1.op;
		idexe.rd = scoreboard.fpMult1.fi;
		idexe.rs = scoreboard.fpMult1.fj;
		idexe.rt = scoreboard.fpMult1.fk;
		idexe.op_A = idexe.rs;
		idexe.op_B = idexe.rt;
		idexe.data = idexe.rs;
		scoreboard.status[1] = Execution;
	}
	if (scoreboard.status[1] == Execution) {	
		Mem_WB writeback = {0};
		writeback.rd = idexe.rd;
		writeback.op_code = idexe.rd;
		if (writeback.op_code == 0)
		{
			wbArray[1] = writeback;
		}
		else {
			writeback.ALU_out = fregs[(int) idexe.rs] * fregs[(int) idexe.rt];
			wbArray[1] = writeback;
		}
		scoreboard.status[1] = Write;
	}
}

// Multiply 2 Pipeline
void updateMult2Pipeline(Mem_WB wbArray[5]) {
	ID_Exe idexe = {0};
	if (scoreboard.fpMult2.rj && scoreboard.fpMult2.rk &&  scoreboard.status[2] == Issue)
	{
		idexe.op_code = scoreboard.fpMult2.op;
		idexe.rd = scoreboard.fpMult2.fi;
		idexe.rs = scoreboard.fpMult2.fj;
		idexe.rt = scoreboard.fpMult2.fk;
		idexe.op_A = idexe.rs;
		idexe.op_B = idexe.rt;
		idexe.data = idexe.rs;
		scoreboard.status[2] = Execution;
	}
	if (scoreboard.status[2] == Execution) {	
		Mem_WB writeback = {0};
		writeback.rd = idexe.rd;
		writeback.op_code = idexe.rd;
		if (writeback.op_code == 0)
		{
			wbArray[2] = writeback;
		}
		else {
			writeback.ALU_out = fregs[(int) idexe.rs] * fregs[(int) idexe.rt];
			wbArray[2] = writeback;
		}
		scoreboard.status[2] = Write;
	}
}

// FAdd Pipeline
void updateAddPipeline(Mem_WB wbArray[5]) {
	ID_Exe idexe = {0};
	if (scoreboard.fpAdd.rj && scoreboard.fpAdd.rk && scoreboard.status[3] == Issue)
	{
		
		idexe.op_code = scoreboard.fpAdd.op;
		idexe.rd = scoreboard.fpAdd.fi;
		idexe.rs = scoreboard.fpAdd.fj;
		idexe.rt = scoreboard.fpAdd.fk;
		idexe.op_A = idexe.rs;
		idexe.op_B = idexe.rt;
		idexe.data = idexe.rs;
		scoreboard.status[3] = Execution;
	}
	if (scoreboard.status[3] == Execution) {	
		Mem_WB writeback = {0};
		writeback.rd = idexe.rd;
		writeback.op_code = idexe.rd;
		if (writeback.op_code == 0)
		{
			wbArray[3] = writeback;
		}
		else {
			if (writeback.op_code == 0x01) {
				writeback.ALU_out = fregs[(int) idexe.rs] + fregs[(int) idexe.rt];
			}
			else {
				writeback.ALU_out = fregs[(int) idexe.rs] - fregs[(int) idexe.rt];
			}
			wbArray[3] = writeback;
		}
		scoreboard.status[3] = Write;
	}
}

// FDiv Pipeline
Mem_WB updateDivPipeline(Mem_WB wbArray[5]) {
	ID_Exe idexe = {0};
	if (scoreboard.fpDivide.rj && scoreboard.fpDivide.rk && scoreboard.status[4] == Issue)
	{
		idexe.op_code = scoreboard.fpDivide.op;
		idexe.rd = scoreboard.fpDivide.fi;
		idexe.rs = scoreboard.fpDivide.fj;
		idexe.rt = scoreboard.fpDivide.fk;
		idexe.op_A = idexe.rs;
		idexe.op_B = idexe.rt;
		idexe.data = idexe.rs;
		scoreboard.status[4] = Execution;
	}
	if (scoreboard.status[4] == Execution) {
		Mem_WB writeback = {0};
		writeback.rd = idexe.rd;
		writeback.op_code = idexe.rd;
		if (writeback.op_code == 0)
		{
			wbArray[4] = writeback;
		}
		else {
			writeback.ALU_out = fregs[(int) idexe.rs] / fregs[(int) idexe.rt];
			wbArray[4] = writeback;
		}
		scoreboard.status[4] = Write;
	}
}

//======END OF PIPELINES======
/*
**
  WRITEBACK
**
*/
void writebackStage(Mem_WB wbArray[5])
{
	int i;
	for (i = 0; i < 5; i++) {
		if (wbArray[i].op_code != 0) {
			if (i == 0) {
				scoreboard.integer.busy = 0;
				regs[(int) wbArray[i].rd] = wbArray[i].ALU_out;
				scoreboard.regResultStatus.status[(int) wbArray[i].rd] = null;
			}
			else {
				if (i == 1) {
					scoreboard.fpMult1.busy = 0;
				}
				if (i == 2) {
					scoreboard.fpMult2.busy = 0;
				}
				if (i == 3) {
					scoreboard.fpAdd.busy = 0;
				}
				if (i == 4) {
					scoreboard.fpDivide.busy = 0;
				}
				fregs[(int) wbArray[i].rd] = wbArray[i].ALU_out;
				scoreboard.regResultStatus.fpStatus[(int) wbArray[i].rd] = null;
			}
		}	
	}
}

/*
**
   OBTAIN FIRST ELEMENT OF ARRAY, THEN DELETE
**
*/
ID_Exe dQ(ID_Exe q[16])
{
	int i;
	ID_Exe idexe;
	idexe = q[0];
	for(i = 0;i < 15;i++) {
		q[i] = q[i+1];
	}
	return idexe;
}
/*
**
   PUSH ELEMENT INTO QUEUE
**
*/
void nQ(ID_Exe q[16], ID_Exe idexe)
{
	int i;
	bool run = true;
	for(i = 0;i < 16;i++) {
		if (run) {
			if (q[i].op_code == 0)
			{
				q[i] = idexe;
				run = false;
			}
		}
	}
}
/*
**
   INITIALIZE QUEUE
**
*/
void iQ(ID_Exe q[16])
{
	ID_Exe idexe;
	idexe.op_code = 0;
	int i;
	for (i = 0; i < 16; i++)
	{
		q[i] = idexe;
	}
}

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
	
	printf("pipeSim (a) or fullSim (b): "); // get a scoreboard type
	char option[50];		// large, arbitrary buffer size
	scanf("%s", option);
	if (strncasecmp(option, "a", 1) == 0) {
		sco = 1;

	}
	else {
		sco = 2;
	}

	char ch; // the current char in the file
	int textBase = 0x20000500; // start of text location in memory
	char opCode[4]; // for storing opcodes
	char regi[4];   // for storing any registers
	char addr[8];	// for storing any addresses
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
					if (ch == 'm') {
						ch = '.';
					}
					regi[j] = ch;
					j++;
				}

				// extract address
				j = 2;
				while (j < 10)
				{
					ch = fgetc(ifp);
					if (ch == 'm') {
						ch = '.';
					}
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
					if (ch == 'm') {
						ch = '.';
					}
					addr[j] = ch;
					j++;
				}
				j = 0;
				while (j < 4)
				{
					ch = fgetc(ifp);
					if (ch == 'm') {
						ch = '.';
					}
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
					if (ch == 'm') {
						ch = '.';
					}
					regi[j] = ch;
					j++;
				}

				// extract address
				j = 2;
				while (j < 10)
				{
					ch = fgetc(ifp);
					if (ch == 'm') {
						ch = '.';
					}
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
					if (ch == 'm') {
						ch = '.';
					}
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
					if (ch == 'm') {
						ch = '.';
					}
					regi[j] = ch;
					j++;
				}
				
				// extract address
				j = 2;
				while (j < 10)
				{
					ch = fgetc(ifp);
					if (ch == 'm') {
						ch = '.';
					}
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
				if (opCode[3] == 'F') {
					int j = 0;
					while (j < 4)
					{
						ch = fgetc(ifp);
						if (ch == 'm') {
							ch = '.';
						}
						regi[j] = ch;
						j++;
					}
	
					int regData = convert(regi);
					int rd = regData;
	
					// extract register
					j = 0;
					while (j < 4)
					{
						ch = fgetc(ifp);
						if (ch == 'm') {
							ch = '.';
						}
						regi[j] = ch;
						j++;
					}
					float fregData =(float) strtof(regi, NULL);
					fregs[rd] = fregData;
				}
				else {
					int opData = convert(opCode);
					write(opData, textBase - 0x500, text);
					textBase++;
					
					// extract register
					int j = 0;
					while (j < 4)
					{
						ch = fgetc(ifp);
						if (ch == 'm') {
							ch = '.';
						}
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
						if (ch == 'm') {
							ch = '.';
						}
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
						if (ch == 'm') {
							ch = '.';
						}
						regi[j] = ch;
						j++;
					}
					regData = convert(regi);
					write(regData, textBase - 0x500, text);
					textBase++;
				}
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
				if (ch == 'm') {
					ch = '.';
				}
				addr[i] = ch;
				i++;
			}
		
			// extract value
			int k = 2;
			while (k < 8)
			{
				ch = fgetc(ifp);
				if (ch == 'm') {
					ch = '.';
				}
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

IF_ID if_stage(int kernel_text[560])
{ // FETCH INSTRUCTION
	IF_ID ifID = {0}; // create latch to return
	ifID.op_code = read(PC - 0x500, kernel_text); // note: instructions are broken up into
	if (ifID.op_code != 0) {
		IC++;	
	}
	ifID.off1 = read(PC - 0x500 + 1, kernel_text); // opCode, relevant data 1, relevant data 2,
	ifID.off2 = read(PC - 0x500 + 2, kernel_text); // and relevant data 3 (these can be registers or addresses)
	ifID.off3 = read(PC - 0x500 + 3, kernel_text); // So while fetching instruction, I store all four into latch
	PC = PC + 4; // update counter
	return ifID;
}

ID_Exe id_stage(IF_ID ir, ID_Exe intQ[16], ID_Exe multQ[16], ID_Exe addQ[16], ID_Exe divQ[16]) 
{ // DECODE INSTRUCTION
	ID_Exe idExe = {0}; // create latch to return
	idExe.op_code = ir.op_code; // set the opcode to the previous instructions op code

	if (idExe.op_code == 0x16 || idExe.op_code == 0x27) {
		idExe.rs = ir.off1; // if BRANCH, need to deal with what instruction is fetched next
		idExe.rt = ir.off2; // set the register source and registers source 2 (because branches can have up to two
	}
	else if (idExe.op_code == 0x38 || idExe.op_code == 0x39) { // BGE
		idExe.rd = ir.off1;
		idExe.rs = ir.off1; 
		idExe.rt = ir.off2;
		idExe.data= ir.off3;
	}
	else if (idExe.op_code == 0x0D || idExe.op_code == 0x05 || idExe.op_code == 0x04 || idExe.op_code == 0x01 || idExe.op_code == 0x02 || 			idExe.op_code == 0x03) { //SUB ADD ADDI FADD FSUB FMUL
		idExe.rd = ir.off1; // Sub, Add, and Addi have three pieces of info
		idExe.rs = ir.off2; // the register destination (rd), register source 1 (rs)
		idExe.rt = ir.off3; // and register source 2 (rt), so these must be extracted from the
		idExe.op_A = idExe.rs; // input latch. rs and rt can also be called op_A and op_B, so
		idExe.op_B = idExe.rt; // these variables are used for clarity. 
		idExe.data = idExe.rt;	// rt can also be the immediate value or address, so data is used for clarity
	}
	else { // Every other instruction only has two pieces of info + LD SD
		idExe.rd = ir.off1; // register destination
		idExe.rs = ir.off2; // register source
		idExe.op_A = idExe.rs; // register source can be called op_A for clarification
		idExe.data = idExe.rs; // immediate/address can be data for clarification
	}
	if (idExe.op_code == 0x01 || idExe.op_code == 0x02)
	{
		idExe.pipeline = 2;
	}
	if (idExe.op_code == 0x03)
	{
		idExe.pipeline = 1;
	}
	if (idExe.op_code != 0) {
		switch (idExe.pipeline)
			{
				case 0:
					nQ(intQ, idExe);
					break;
				case 1:
					nQ(multQ, idExe);
					break;
				case 2:
					nQ(addQ, idExe);
					break;
				case 3:
					nQ(divQ, idExe);
					break;
			}
	}
//==================Scoreboard Update============================
//Integer
  int cont = 0;
  ID_Exe test = intQ[0];
  if (test.op_code != 0) {
  	cont = 0;
	if (sco == 1) {
    		if (scoreboard.status[0] != Issue) {
  	    		cont = 1;
  	 	}
  	}
  	else if (sco == 2) {
  		if (!scoreboard.integer.busy) {
  			cont = 1;
  		}
  	}
  	if (cont) {
  		dQ(intQ);
  		scoreboard.status[0] = Issue;
    		scoreboard.integer.busy = 1;
    		scoreboard.integer.op = test.op_code;
    		scoreboard.integer.fi = test.rd;
    		scoreboard.integer.fj = test.rs;
    		scoreboard.integer.fk = test.rt;
    		scoreboard.integer.qj = scoreboard.regResultStatus.status[(int) test.rs];
    		scoreboard.integer.qk = scoreboard.regResultStatus.status[(int) test.rt];
    		scoreboard.integer.rj = 0;
    		if (scoreboard.regResultStatus.status[(int) test.rs] == null) {
      			scoreboard.integer.rj = 1;
    		}
    		scoreboard.integer.rk = 0;
    		if (scoreboard.regResultStatus.status[(int) test.rt] == null) {
      			scoreboard.integer.rk = 1;
    		}
  	}
   }
  //Mult1
   test = multQ[0];
   if (test.op_code != 0) {
  	cont = 0;
  	if (sco == 1) {
    		if (scoreboard.status[1] != Issue) {
        		cont = 1;
    		}
  	}
  	else if (sco == 2) {
    		if (!scoreboard.fpMult1.busy) {
      			cont = 1;
    		}
  	}
  	if (cont) {
    		dQ(multQ);
		scoreboard.status[1] = Issue;
   		scoreboard.fpMult1.busy = 1;
   	 	scoreboard.fpMult1.op = test.op_code;
  	  	scoreboard.fpMult1.fi = test.rd;
  	  	scoreboard.fpMult1.fj = test.rs;
  	  	scoreboard.fpMult1.fk = test.rt;
  	  	scoreboard.fpMult1.qj = scoreboard.regResultStatus.fpStatus[(int) test.rs];
  	  	scoreboard.fpMult1.qk = scoreboard.regResultStatus.fpStatus[(int) test.rt];
  	  	scoreboard.fpMult1.rj = 0;
  	  	if (scoreboard.regResultStatus.fpStatus[(int) test.rs] == null) {
  	  		scoreboard.fpMult1.rj = 1;
  	  	}
  	  	scoreboard.fpMult1.rk = 0;
  	  	if (scoreboard.regResultStatus.fpStatus[(int) test.rt] == null) {
  	  		scoreboard.fpMult1.rk = 1;
  	  	}
  	}
      }
  	//Mult2
      test = multQ[0];
      if (test.op_code != 0) {
  	cont = 0;
  	if (sco == 1) {
  		if (scoreboard.status[2] != Issue) {
  			cont = 1;
  	  	}
  	}
  	else if (sco == 2) {
  		if (!scoreboard.fpMult2.busy) {
  			cont = 1;
  	  	}
  	}
  	if (cont) {
  		dQ(multQ);
  		scoreboard.status[2] = Issue;
  	  	scoreboard.fpMult2.busy = 1;
  	  	scoreboard.fpMult2.op = test.op_code;
  	  	scoreboard.fpMult2.fi = test.rd;
  	  	scoreboard.fpMult2.fj = test.rs;
  	  	scoreboard.fpMult2.fk = test.rt;
  	  	scoreboard.fpMult2.qj = scoreboard.regResultStatus.fpStatus[(int) test.rs];
  	  	scoreboard.fpMult2.qk = scoreboard.regResultStatus.fpStatus[(int) test.rt];
  	  	scoreboard.fpMult2.rj = 0;
  	  	if (scoreboard.regResultStatus.fpStatus[(int) test.rs] == null) {
  	  		scoreboard.fpMult2.rj = 1;
  	  	}
  	  	scoreboard.fpMult2.rk = 0;
  	  	if (scoreboard.regResultStatus.fpStatus[(int) test.rt] == null) {
  	  		scoreboard.fpMult2.rk = 1;
  	  	}
  	}
	}
  	//Add
  	test = addQ[0];
	if (test.op_code != 0) {
  	cont = 0;
  	if (sco == 1) {
  		if (scoreboard.status[3] != Issue) {
  	    		cont = 1;
  	  	}
  	}
  	else if (sco == 2) {
  		if (!scoreboard.fpAdd.busy) {
  	    		cont = 1;
  	  	}
  	}
  	if (cont) {
  		dQ(addQ);
  	  	scoreboard.status[3] = Issue;
  	  	scoreboard.fpAdd.busy = 1;
  	  	scoreboard.fpAdd.op = test.op_code;
  	  	scoreboard.fpAdd.fi = test.rd;
  	  	scoreboard.fpAdd.fj = test.rs;
  	  	scoreboard.fpAdd.fk = test.rt;
  	  	scoreboard.fpAdd.qj = scoreboard.regResultStatus.fpStatus[(int) test.rs];
  	  	scoreboard.fpAdd.qk = scoreboard.regResultStatus.fpStatus[(int) test.rt];
  	  	scoreboard.fpAdd.rj = 0;
  	  	if (scoreboard.regResultStatus.fpStatus[(int) test.rs] == null) {
  	  		scoreboard.fpAdd.rj = 1;
  	  	}
  	  	scoreboard.fpAdd.rk = 0;
  	  	if (scoreboard.regResultStatus.fpStatus[(int) test.rt] == null) {
  	    		scoreboard.fpAdd.rk = 1;
  	  	}
  	}
	}
  	//Div
  	test = divQ[0];
	if (test.op_code != 0) {
  	cont = 0;
  	if (sco == 1) {
  		if (scoreboard.status[4] != Issue) {
  			cont = 1;
  		}
  	}
  	else if (sco == 2) {
  	  	if (!scoreboard.fpDivide.busy) {
  			cont = 1;
  		}
  	}
  	if (cont) {
  	  	dQ(addQ);
  	  	scoreboard.status[4] = Issue;
  	  	scoreboard.fpDivide.busy = 1;
  	  	scoreboard.fpDivide.op = test.op_code;
  	  	scoreboard.fpDivide.fi = test.rd;
  	  	scoreboard.fpDivide.fj = test.rs;
  	  	scoreboard.fpDivide.fk = test.rt;
  		scoreboard.fpDivide.qj = scoreboard.regResultStatus.fpStatus[(int) test.rs];
  	  	scoreboard.fpDivide.qk = scoreboard.regResultStatus.fpStatus[(int) test.rt];
  	  	scoreboard.fpDivide.rj = 0;
  	  	if (scoreboard.regResultStatus.fpStatus[(int) test.rs] == null) {
  	    		scoreboard.fpDivide.rj = 1;
  	  	}
          	scoreboard.fpDivide.rk = 0;
  		if (scoreboard.regResultStatus.fpStatus[(int) test.rt] == null) {
  			scoreboard.fpDivide.rk = 1;
  		}
	}
	
    }	
//============================================================
}

/*
**
   MAIN FUNCTION
**
*/	

int main()
{
	int i;
	for (i = 0; i < 32; i++) {
		if (i < 30) {
			scoreboard.regResultStatus.fpStatus[i] = null;
		}
		scoreboard.regResultStatus.status[i] = null;	
	}
	regs[0] = 0; // set reg 0 to 0 for reasons (apparently this is default)
	loaderModule(kernel_text, kernel_data); // load binary encoded file
	ID_Exe intQ[16];
	ID_Exe multQ[16];
	ID_Exe addQ[16];
	ID_Exe divQ[16];
	Mem_WB wbArrayOld[5];
	Mem_WB wbArrayNew[5];
	iQ(intQ);
	iQ(multQ);
	iQ(addQ);
	iQ(divQ);
	IF_ID IF_ID_new = {0}; 
	IF_ID IF_ID_old = {0}; // create old/new version of latches
	Exe_Mem Exe_Mem_new = {0};
	Exe_Mem Exe_Mem_old = {0};
	Mem_WB Mem_WB_new = {0};
	Mem_WB Mem_WB_old = {0};
	run = true; // user_mode is on
	while(run) {
		int i;		
		for (i = 0; i < 5; i++) {
			wbArrayOld[i] = wbArrayNew[i];	
		}
		writebackStage(wbArrayOld);
		updateMult1Pipeline(wbArrayNew);
		updateMult2Pipeline(wbArrayNew);
		updateDivPipeline(wbArrayNew);
		updateAddPipeline(wbArrayNew);
		Exe_Mem_old = Exe_Mem_new;
		Exe_Mem_new = updateIntPipeline1();
		updateIntPipeline2(wbArrayNew, Exe_Mem_old);

		IF_ID_old = IF_ID_new; // update IF_ID
		IF_ID_new = if_stage(kernel_text); // fetch instruction
		id_stage(IF_ID_old, intQ, multQ, addQ, divQ);
		CC++; // update clock cycle
	}	
}
