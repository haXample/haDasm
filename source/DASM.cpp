// haDASM - Disassembler for Microchip processors
// dasm.cpp - C++ Developer source file.
// (c)1990-2023 by helmut altmann
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file COPYING.  If not, write to
// the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.

#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <stdio.h>
#include <io.h>
#include <shlwapi.h>   // Library shlwapi.lib for PathFileExistsA
#include <conio.h>     // For _putch(), _getch() ..

#include <shlwapi.h>   // StrStrI, StrCmpI, StrCmpNI

#include <ctype.h>
#include <string.h>
#include <windows.h>   // For console specific functions

#define ERR        -1
#define TRUE        1
#define FALSE       0
#define LENGTH     50

#define ESC     0x1B
#define SPACE   ' '

#define ROMSIZE  32*1024

// Global variables
char* signon  = "M68HC05 Disassembler, V2.0\n";

char argv_name[LENGTH+5];
char inbuf[ROMSIZE+1];

int fh,i,j,k,l;
int bytesrd;

// Forward declaration of functions included in this code module:
void DebugPrintBuffer(char *, int);
void DebugStop(char*, int);

typedef struct tag_68HC05INS {
  char*  mneStr;
} _6805MNEMONIC, *LP_6805MNEMONIC;

// --------------------------------------------
// Motorola M68HC05 Family Instruction mnemonic
// --------------------------------------------
// IMM Immediate addressing mode 
//     ii = Immediate 8bit operand (# Immediate value)
// INH Inherent addressing mode
//     no operand
// DIR Direct addressing mode
//     dd = Direct 8bit address of operand
// REL Relative addressing mode
//     rr = relative 8bit offset of branch instruction 
// EXT Extended addressing mode
//     hh ll = High and low bytes of 16bit operand address 
// IX  Indexed, no offset addressing mode
//     no operand
// IX1 Indexed, 8-bit offset addressing mode
//     ff = byte offset in indexed, 8-bit offset addressing
// IX2 Indexed, 16-bit offset addressing mode
//     ee ff High and low bytes of offset in indexed, 16-bit offset addressing 
// 
_6805MNEMONIC mnemonic6805[] = {
  // DIR-Addr-Mode: 3-Byte-Instr., op1={0..7}, op2={dd}, op3={rr}
  {"\t\t 5~\tbrset\t0,$"}, // 00  {6,"BRSET",8,0x00},
  {"\t\t 5~\tbrclr\t0,$"}, // 01  {6,"BRCLR",8,0x01},
  {"\t\t 5~\tbrset\t1,$"}, // 02  {6,"BRSET",8,0x02},
  {"\t\t 5~\tbrclr\t1,$"}, // 03  {6,"BRCLR",8,0x03},
  {"\t\t 5~\tbrset\t2,$"}, // 04  {6,"BRSET",8,0x04},
  {"\t\t 5~\tbrclr\t2,$"}, // 05  {6,"BRCLR",8,0x05},
  {"\t\t 5~\tbrset\t3,$"}, // 06  {6,"BRSET",8,0x06},
  {"\t\t 5~\tbrclr\t3,$"}, // 07  {6,"BRCLR",8,0x07},
  {"\t\t 5~\tbrset\t4,$"}, // 08  {6,"BRSET",8,0x08},
  {"\t\t 5~\tbrclr\t4,$"}, // 09  {6,"BRCLR",8,0x09},
  {"\t\t 5~\tbrset\t5,$"}, // 0A  {6,"BRSET",8,0x0A},
  {"\t\t 5~\tbrclr\t5,$"}, // 08  {6,"BRCLR",8,0x08},
  {"\t\t 5~\tbrset\t6,$"}, // 0C  {6,"BRSET",8,0x0C},
  {"\t\t 5~\tbrclr\t6,$"}, // 0D  {6,"BRCLR",8,0x0D},
  {"\t\t 5~\tbrset\t7,$"}, // 0E  {6,"BRSET",8,0x0E},
  {"\t\t 5~\tbrclr\t7,$"}, // 0F  {6,"BRCLR",8,0x0F},
             
  // DIR-Addr-Mode: 2-Byte-Instr., op1={0..7}, op2={dd}
  {"\t 5~\tbset\t0,$"},    // 10  {5,"BSET",7,0x10},
  {"\t 5~\tbclr\t0,$"},    // 11  {5,"BCLR",7,0x11},
  {"\t 5~\tbset\t1,$"},    // 12  {5,"BSET",7,0x12},
  {"\t 5~\tbclr\t1,$"},    // 13  {5,"BCLR",7,0x13},
  {"\t 5~\tbset\t2,$"},    // 14  {5,"BSET",7,0x14},
  {"\t 5~\tbclr\t2,$"},    // 15  {5,"BCLR",7,0x15},
  {"\t 5~\tbset\t3,$"},    // 16  {5,"BSET",7,0x16},
  {"\t 5~\tbclr\t3,$"},    // 17  {5,"BCLR",7,0x17},
  {"\t 5~\tbset\t4,$"},    // 18  {5,"BSET",7,0x18},
  {"\t 5~\tbclr\t4,$"},    // 19  {5,"BCLR",7,0x19},
  {"\t 5~\tbset\t5,$"},    // 1A  {5,"BSET",7,0x1A},
  {"\t 5~\tbclr\t5,$"},    // 18  {5,"BCLR",7,0x18},
  {"\t 5~\tbset\t6,$"},    // 1C  {5,"BSET",7,0x1C},
  {"\t 5~\tbclr\t6,$"},    // 1D  {5,"BCLR",7,0x1D},
  {"\t 5~\tbset\t7,$"},    // 1E  {5,"BSET",7,0x1E},
  {"\t 5~\tbclr\t7,$"},    // 1F  {5,"BCLR",7,0x1F},

  // REL-Addr-Mode: 2-Byte-Instr., op1={rr}
  {"\t 3~\tbra\t$"},       // 20  {4,"BRA", 3,0x20},
  {"\t 3~\tbrn\t$"},       // 21  {4,"BRN", 3,0x21},
  {"\t 3~\tbhi\t$"},       // 22  {4,"BHI", 3,0x22},
  {"\t 3~\tbls\t$"},       // 23  {4,"BLS", 3,0x23},
  {"\t 3~\tbcc\t$"},       // 24  {4,"BCC", 3,0x24},  {4,"BHS"}, 3,0x24}, // same as BCC
  {"\t 3~\tbcs\t$"},       // 25  {4,"BCS", 3,0x25},  {4,"BLO"}, 3,0x25}, // same as BCS
  {"\t 3~\tbne\t$"},       // 26  {4,"BNE", 3,0x26},
  {"\t 3~\tbeq\t$"},       // 27  {4,"BEQ", 3,0x27},
  {"\t 3~\tbhcc\t$"},      // 28  {5,"BHCC",3,0x28},
  {"\t 3~\tbhcs\t$"},      // 29  {5,"BHCS",3,0x29},
  {"\t 3~\tbpl\t$"},       // 2A  {4,"BPL", 3,0x2A},
  {"\t 3~\tbmi\t$"},       // 2B  {4,"BMI", 3,0x2B},
  {"\t 3~\tbmc\t$"},       // 2C  {4,"BMC", 3,0x2C},
  {"\t 3~\tbms\t$"},       // 2D  {4,"BMS", 3,0x2D},
  {"\t 3~\tbil\t$"},       // 2E  {4,"BIL", 3,0x2E},
  {"\t 3~\tbih\t$"},       // 2F  {4,"BIH", 3,0x2F},

  // DIR-Addr-Mode: 2-Byte-Instr., op1={dd}
  {"\t 5~\tneg\t$"},       // 30  {4,"NEG",4,0x30},
  {" \t---"},              // 31
  {" \t---"},              // 32
  {"\t 5~\tcom\t$"},       // 33  {4,"COM",4,0x33},
  {"\t 5~\tlsr\t$"},       // 34  {4,"LSR",4,0x34},
  {" \t---"},              // 35
  {"\t 5~\tror\t$"},       // 36  {4,"ROR",4,0x36},
  {"\t 5~\tasr\t$"},       // 37  {4,"ASR",4,0x37},
  {"\t 5~\tlsl\t$"},       // 38  {4,"LSL",4,0x38},   {4,"ASL",4,0x38}, // same as LSL
  {"\t 5~\trol\t$"},       // 39  {4,"ROL",4,0x39},
  {"\t 5~\tdec\t$"},       // 3A  {4,"DEC",4,0x3A},
  {" \t---"},              // 3B
  {"\t 5~\tinc\t$"},       // 3C  {4,"INC",4,0x3C},
  {"\t 4~\ttst\t$"},       // 3D  {4,"TST",4,0x3D},
  {" \t---"},              // 3E
  {"\t 5~\tclr\t$"},       // 3F  {4,"CLR",4,0x3F},

  // INH-Addr-Mode: 1-Byte-Instr., no operands
  {" 3~\tnega"},           // 40  {5,"NEGA",2,0x40},
  {" \t---"},              // 41
  {"11~\tmul"},            // 42  {4,"MUL"},2,0x42},
  {" 3~\tcoma"},           // 43  {5,"COMA",2,0x43},
  {" 3~\tlsra"},           // 44  {5,"LSRA",2,0x44},
  {" \t---"},              // 45
  {" 3~\trora"},           // 46  {5,"RORA",2,0x46},  
  {" 3~\tasra"},           // 47  {5,"ASRA",2,0x47},
  {" 3~\tlsla"},           // 48  {5,"LSLA",2,0x48},  {5,"ASLA",2,0x48},  // same as LSLA
  {" 3~\trola"},           // 49  {5,"ROLA",2,0x49},
  {" 3~\tdeca"},           // 4A  {5,"DECA",2,0x4A},
  {" \t---"},              // 4B  
  {" 3~\tinca"},           // 4C  {5,"INCA",2,0x4C},
  {" 3~\ttsta"},           // 4D  {5,"TSTA",2,0x4D},
  {" \t---"},              // 4E  
  {" 3~\tclra"},           // 4F  {5,"CLRA",2,0x4F},

  // INH-Addr-Mode: 1-Byte-Inst., no operands
  {" 3~\tnegx"},           // 50  {5,"NEGX",2,0x50},
  {" \t---"},              // 51
  {" \t---"},              // 52  
  {" 3~\tcomx"},           // 53  {5,"COMX",2,0x53},
  {" 3~\tlsrx"},           // 54  {5,"LSRX",2,0x54},
  {" \t---"},              // 55
  {" 3~\trorx"},           // 56  {5,"RORX",2,0x56},                          
  {" 3~\tasrx"},           // 57  {5,"ASRX",2,0x57},
  {" 3~\tlslx"},           // 58  {5,"LSLX",2,0x58},  {5,"ASLX",2,0x58},  // same as LSLX
  {" 3~\trolx"},           // 59  {5,"ROLX",2,0x59},
  {" 3~\tdecx"},           // 5A  {5,"DECX",2,0x5A},
  {" \t---"},              // 5B  
  {" 3~\tincx"},           // 5C  {5,"INCX",2,0x5C},
  {" 3~\ttstx"},           // 5D  {5,"TSTX",2,0x5D},
  {" \t---"},              // 5E  
  {" 3~\tclrx"},           // 5F  {5,"CLRX",2,0x5F},
  
  // IX1-Addr-Mode: 2-Byte-Instr., op1={ff}, op2={X}
  {"\t 6~\tneg\t$"},       // 60  {4,"NEG",4,0x60},
  {" \t---"},              // 61
  {" \t---"},              // 62
  {"\t 6~\tcom\t$"},       // 63  {4,"COM",4,0x62},
  {"\t 6~\tlsr\t$"},       // 64  {4,"LSR",4,0x64},
  {" \t---"},              // 65
  {"\t 6~\tror\t$"},       // 66  {4,"ROR",4,0x66},
  {"\t 6~\tasr\t$"},       // 67  {4,"ASR",4,0x67},
  {"\t 6~\tlsl\t$"},       // 68  {4,"LSL",4,0x68},   {4,"ASL",4,0x68}, // same as LSL
  {"\t 6~\trol\t$"},       // 69  {4,"ROL",4,0x69},
  {"\t 6~\tdec\t$"},       // 6A  {4,"DEC",4,0x6A},
  {" \t---"},              // 6B
  {"\t 6~\tinc\t$"},       // 6C  {4,"INC",4,0x6C},
  {"\t 5~\ttst\t$"},       // 6D  {4,"TST",4,0x6D},
  {" \t---"},              // 6E
  {"\t 6~\tclr\t$"},       // 6F  {4,"CLR",4,0x6F},

  // IX-Addr-Mode: 1-Byte-Instr., no operands
  {" 5~\tneg\t,x"},       // 70 {4,"NEG",4,0x70},
  {" \t---"},             // 71
  {" \t---"},             // 72
  {" 5~\tcom\t,x"},       // 73 {4,"COM",4,0x73},
  {" 5~\tlsr\t,x"},       // 74 {4,"LSR",4,0x74},
  {" \t---"},             // 75
  {" 5~\tror\t,x"},       // 76 {4,"ROR",4,0x76},
  {" 5~\tasr\t,x"},       // 77 {4,"ASR",4,0x77},
  {" 5~\tlsl\t,x"},       // 78 {4,"LSL",4,0x78},   {4,"ASL",4,0x78}, // same as LSL
  {" 5~\trol\t,x"},       // 79 {4,"ROL",4,0x79},
  {" 5~\tdec\t,x"},       // 7A {4,"DEC",4,0x7A},
  {" \t---"},             // 7B
  {" 5~\tinc\t,x"},       // 7C {4,"INC",4,0x7C},
  {" 4~\ttst\t,x"},       // 7D {4,"TST",4,0x7D},
  {" \t---"},             // 7E
  {" 5~\tclr\t,x"},       // 7F {4,"CLR",4,0x7F},

  // INH-Addr-Mode: 1-Byte-Inst., no operands
  {" 9~\trti"},            // 80  {4,"RTI"}, 2,0x80},
  {" 6~\trts"},            // 81  {4,"RTS"}, 2,0x81},
  {" \t---"},              // 82
  {"10~\tswi"},            // 83  {4,"SWI"}, 2,0x83},
  {" \t---"},              // 84
  {" \t---"},              // 85
  {" \t---"},              // 86
  {" \t---"},              // 87
  {" \t---"},              // 88
  {" \t---"},              // 89
  {" \t---"},              // 8A
  {" \t---"},              // 8B
  {" \t---"},              // 8C
  {" \t---"},              // 8D
  {" 2~\tstop"},           // 8E  {5,"STOP",2,0x8E},
  {" 2~\twait"},           // 8F  {5,"WAIT",2,0x8F},
  
  // INH-Addr-Mode: 1-Byte-Inst., no operands
  {" \t---"},              // 90
  {" \t---"},              // 91
  {" \t---"},              // 92
  {" \t---"},              // 93
  {" \t---"},              // 94
  {" \t---"},              // 95
  {" \t---"},              // 96
  {" 2~\ttax"},            // 97  {4,"TAX", 2,0x97},
  {" 2~\tclc"},            // 98  {4,"CLC", 2,0x98},
  {" 2~\tsec"},            // 99  {4,"SEC", 2,0x99},
  {" 2~\tcli"},            // 9A  {4,"CLI", 2,0x9A},
  {" 2~\tsei"},            // 9B  {4,"SEI", 2,0x9B},
  {" 2~\trsp"},            // 9C  {4,"RSP", 2,0x9C},
  {" 2~\tnop"},            // 9D  {4,"NOP", 2,0x9D},
  {" \t---"},              // 9E  
  {" 2~\ttxa"},            // 9F  {4,"TXA", 2,0x9F},
             
  // IMM-Addr-Mode: 2-Byte-Instr. op1={#ii}
  {"\t 2~\tsub\t#$"},      // A0  {4,"SUB",6,0xA0},
  {"\t 2~\tcmp\t#$"},      // A1  {4,"CMP",6,0xA1},
  {"\t 2~\tcpx\t#$"},      // A2  {4,"CPX",6,0xA3},
  {"\t 2~\tsbc\t#$"},      // A3  {4,"SBC",6,0xA2},
  {"\t 2~\tand\t#$"},      // A4  {4,"AND",6,0xA4},
  {"\t 2~\tbit\t#$"},      // A5  {4,"BIT",6,0xA5},
  {"\t 2~\tlda\t#$"},      // A6  {4,"LDA",6,0xA6},
  {" \t---"},              // A7  "STA" no #-mode 
  {"\t 2~\teor\t#$"},      // A8  {4,"EOR",6,0xA8},
  {"\t 2~\tadc\t#$"},      // A9  {4,"ADC",6,0xA9},
  {"\t 2~\tora\t#$"},      // AA  {4,"ORA",6,0xAA},
  {"\t 2~\tadd\t#$"},      // AB  {4,"ADD",6,0xAB},
  {" \t---"},              // AC  "JSR" no #-mode
  // REL-Addr-mode: 2-Byte-Instr., op1={rr}
  {"\t 6~\tbsr\t$"},       // AD  {4,"BSR",3,0xAD},
  // IMM-Addr-mode: 2-Byte-Instr. op1={#ii}
  {"\t 2~\tldx\t#$"},      // AE  {4,"LDX",6,0xAE},
  {" \t---"},              // AF  "STX" no #-mode
  
   // DIR-Addr-Mode: 2-Byte-Instr., op1={dd}, op2={rr}
  {"\t 3~\tsub\t$"},       // B0  {4,"SUB",6,0xB0},
  {"\t 3~\tcmp\t$"},       // B1  {4,"CMP",6,0xB1},
  {"\t 3~\tsbc\t$"},       // B2  {4,"SBC",6,0xB2},
  {"\t 3~\tcpx\t$"},       // B3  {4,"CPX",6,0xB3},
  {"\t 3~\tand\t$"},       // B4  {4,"AND",6,0xB4},
  {"\t 3~\tbit\t$"},       // B5  {4,"BIT",6,0xB5},
  {"\t 3~\tlda\t$"},       // B6  {4,"LDA",6,0xB6},
  {"\t 4~\tsta\t$"},       // B7  {4,"STA",5,0xB7}, 
  {"\t 3~\teor\t$"},       // B8  {4,"EOR",6,0xB8},
  {"\t 3~\tadc\t$"},       // B9  {4,"ADC",6,0xB9},
  {"\t 3~\tora\t$"},       // BA  {4,"ORA",6,0xBA},
  {"\t 3~\tadd\t$"},       // BB  {4,"ADD",6,0xBB},
  {"\t 2~\tjmp\t$"},       // BC  {4,"JMP",5,0xBC}, 
  {"\t 5~\tjsr\t$"},       // BE  {4,"JSR",5,0xBD},
  {"\t 3~\tldx\t$"},       // BD  {4,"LDX",6,0xBE},
  {"\t 4~\tstx\t$"},       // BF  {4,"STX",5,0xBF},
  
  // EXT-Addr-Mode: 3-Byte-Instr., op1={hh}, op2={ll}
  {"\t\t 4~\tsub\t$"},     // C0  {4,"SUB",6,0xC0},
  {"\t\t 4~\tcmp\t$"},     // C1  {4,"CMP",6,0xC1},
  {"\t\t 4~\tsbc\t$"},     // C2  {4,"SBC",6,0xC2},
  {"\t\t 4~\tcpx\t$"},     // C3  {4,"CPX",6,0xC3},
  {"\t\t 4~\tand\t$"},     // C4  {4,"AND",6,0xC4},
  {"\t\t 4~\tbit\t$"},     // C5  {4,"BIT",6,0xC5},
  {"\t\t 4~\tlda\t$"},     // C6  {4,"LDA",6,0xC6},
  {"\t\t 5~\tsta\t$"},     // C7  {4,"STA",5,0xC7}, 
  {"\t\t 4~\teor\t$"},     // C8  {4,"EOR",6,0xC8},
  {"\t\t 4~\tadc\t$"},     // C9  {4,"ADC",6,0xC9},
  {"\t\t 4~\tora\t$"},     // CA  {4,"ORA",6,0xCA},
  {"\t\t 4~\tadd\t$"},     // CB  {4,"ADD",6,0xCB},
  {"\t\t 3~\tjmp\t$"},     // CC  {4,"JMP",5,0xCC}, 
  {"\t\t 6~\tjsr\t$"},     // CE  {4,"JSR",6,0xCD},
  {"\t\t 4~\tldx\t$"},     // CD  {4,"LDX",3,0xCE},
  {"\t\t 5~\tstx\t$"},     // CF  {4,"STX",5,0xCF},
  
  // IX2-Addr-Mode: 3-Byte-Instr., op1={ee}, op2={ff}
  {"\t\t 5~\tsub\t$"},     // D0  {4,"SUB",6,0xD0},
  {"\t\t 5~\tcmp\t$"},     // D1  {4,"CMP",6,0xD1},
  {"\t\t 5~\tsbc\t$"},     // D2  {4,"SBC",6,0xD2},
  {"\t\t 5~\tcpx\t$"},     // D3  {4,"CPX",6,0xD3},
  {"\t\t 5~\tand\t$"},     // D4  {4,"AND",6,0xD4},
  {"\t\t 5~\tbit\t$"},     // D5  {4,"BIT",6,0xD5},
  {"\t\t 5~\tlda\t$"},     // D6  {4,"LDA",6,0xD6},
  {"\t\t 6~\tsta\t$"},     // D7  {4,"STA",5,0xD7}, 
  {"\t\t 5~\teor\t$"},     // D8  {4,"EOR",6,0xD8},
  {"\t\t 5~\tadc\t$"},     // D9  {4,"ADC",6,0xD9},
  {"\t\t 5~\tora\t$"},     // DA  {4,"ORA",6,0xDA},
  {"\t\t 5~\tadd\t$"},     // DB  {4,"ADD",6,0xDB},
  {"\t\t 4~\tjmp\t$"},     // DC  {4,"JMP",5,0xDC}, 
  {"\t\t 7~\tjsr\t$"},     // DE  {4,"JSR",6,0xDD},
  {"\t\t 5~\tldx\t$"},     // DD  {4,"LDX",3,0xDE},
  {"\t\t 6~\tstx\t$"},     // DF  {4,"STX",5,0xDF},
  
  // IX1-Addr-Mode: 2-Byte-Instr., op1={ff}, op2={X}
  {"\t 4~\tsub\t$"},       // E0  {4,"SUB",6,0xE0},
  {"\t 4~\tcmp\t$"},       // E1  {4,"CMP",6,0xE1},
  {"\t 4~\tsbc\t$"},       // E2  {4,"SBC",6,0xE2},
  {"\t 4~\tcpx\t$"},       // E3  {4,"CPX",6,0xE3},
  {"\t 4~\tand\t$"},       // E4  {4,"AND",6,0xE4},
  {"\t 4~\tbit\t$"},       // E5  {4,"BIT",6,0xE5},
  {"\t 4~\tlda\t$"},       // E6  {4,"LDA",6,0xE6},
  {"\t 5~\tsta\t$"},       // E7  {4,"STA",5,0xE7}, 
  {"\t 4~\teor\t$"},       // E8  {4,"EOR",6,0xE8},
  {"\t 4~\tadc\t$"},       // E9  {4,"ADC",6,0xE9},
  {"\t 4~\tora\t$"},       // EA  {4,"ORA",6,0xEA},
  {"\t 4~\tadd\t$"},       // EB  {4,"ADD",6,0xEB},
  {"\t 3~\tjmp\t$"},       // EC  {4,"JMP",5,0xEC}, 
  {"\t 6~\tjsr\t$"},       // EE  {4,"JSR",6,0xED},
  {"\t 4~\tldx\t$"},       // ED  {4,"LDX",3,0xEE},
  {"\t 5~\tstx\t$"},       // EF  {4,"STX",5,0xEF},
  
  // IX-Addr-Mode: 1-Byte-Instr., no operands
  {" 3~\tsub\t,x"},        // F0  {4,"SUB",6,0xF0},
  {" 3~\tcmp\t,x"},        // F1  {4,"CMP",6,0xF1},
  {" 3~\tsbc\t,x"},        // F2  {4,"SBC",6,0xF2},
  {" 3~\tcpx\t,x"},        // F3  {4,"CPX",6,0xF3},
  {" 3~\tand\t,x"},        // F4  {4,"AND",6,0xF4},
  {" 3~\tbit\t,x"},        // F5  {4,"BIT",6,0xF5},
  {" 3~\tlda\t,x"},        // F6  {4,"LDA",6,0xF6},
  {" 4~\tsta\t,x"},        // F7  {4,"STA",5,0xF7}, 
  {" 3~\teor\t,x"},        // F8  {4,"EOR",6,0xF8},
  {" 3~\tadc\t,x"},        // F9  {4,"ADC",6,0xF9},
  {" 3~\tora\t,x"},        // FA  {4,"ORA",6,0xFA},
  {" 3~\tadd\t,x"},        // FB  {4,"ADD",6,0xFB},
  {" 2~\tjmp\t,x"},        // FC  {4,"JMP",5,0xFC}, 
  {" 5~\tjsr\t,x"},        // FE  {4,"JSR",6,0xFD},
  {" 3~\tldx\t,x"},        // FD  {4,"LDX",3,0xFE},
  {" 4~\tstx\t,x"},        // FF  {4,"STX",5,0xFF},
  }; // //  end-of-table _mnemonic68O5[]

/*****************************************************************
**                                                              **
** Function: main                                               **
**                                                              **
** Abstract: The command Line is read and checked for the       **
**           input file name. If no filename was found          **
**           an error exit will be performed.                   **
**                                                              **
** Extern Input: int argc, char *argv[] from command line       **
**                                                              **
** Extern Ouput: console                                        **
**                                                              **
** Author:  ha                                                  **
** Date:    07.02.1990, 01.08.2023                              **
** Version: 2.0                                                 **
**                                                              **
*****************************************************************/
void main(int argc, char *argv[])
  {
  if (argc != 2) // Illegal parameter, display help             
    {
    printf(signon);
    printf("Usage: %s file.bin [>file.txt]\n", PathFindFileName(argv[0]));                               
    exit(1);
    }

  // get argv_name and convert to upper case chars
  for (i=0; i<LENGTH; i++) argv[1][i] = toupper(argv[1][i]);
  strcpy(argv_name, argv [1]);
  argv_name[LENGTH+4] = 0;    // ensure NUL at string end

  // check and open input file
  if ((fh=open(argv_name, O_RDONLY|O_BINARY)) == ERR)
    {
    printf("Open failed on %s\n", argv_name);
    exit(1);
    }

  bytesrd = read(fh, inbuf, ROMSIZE+1);
  printf("Disassembly of %s\n\n", argv_name);

  // -------- Disassemble MC6805 binary file --------
  //
  i=0; k=0;
  
  while (i<bytesrd)
    {
    printf("%04X  %02X ", i, (UCHAR)inbuf[i]);  // print address & instruction opcode
    j = (UCHAR)inbuf[i];                        // get instruction mnemonic index

    if (mnemonic6805[j].mneStr[0] == '\t')
      {
      // 3 byte instruction "\t\t"
      if (mnemonic6805[j].mneStr[1] == '\t' && (bytesrd-i) >= 3)  
        {
        i++;
        printf("%02X ", (UCHAR)inbuf[i]);       // print operand 1
        i++;
        printf("%02X", (UCHAR)inbuf[i]);        // print operand 2
        printf(mnemonic6805[j].mneStr);         // print mnemonics

        if (j>=0x00 && j<=0x0F)                 // bit test and branch
          {
          printf("%03X", (UCHAR)inbuf[i-1]);    // print 8bit RAM location address
          if (inbuf[i] < 0x80) l = i+1 + (int)inbuf[i];
          else l = i+128 + (int)(inbuf[i] | 0xFF00);
          printf(",$%04X", (WORD)l);            // ",$" append 16bit branch address
          }
        else                                    // print 16bit location address
          printf("%02X%02X", (UCHAR)inbuf[i-1], (UCHAR)inbuf[i]);
        
        if (j>=0xD0 && j<=0xDF) printf(",x");   // indexed 16bit offset

        // For the sake of legibility:
        // Insert a blank line after unconditional JMPs
        if (!StrCmpNI(&mnemonic6805[j].mneStr[6], "jmp",3)) printf("\n");
        } // end if "\t\t"
      
      // 2 byte instruction "\t"
      else if ((bytesrd-i) >= 2)                
        {                                       
        i++;                                   
        printf("%02X\t", (UCHAR)inbuf[i]);      // print operand byte

        // Check if two odd last bytes were left for a 3 byte instruction
        if (mnemonic6805[j].mneStr[1] == '\t') 
          printf("\t\t---\t\t\t; FCB  $%02X, $", (UCHAR)inbuf[i+1]);
        else
          printf(&mnemonic6805[j].mneStr[0]);   // print mnemonics

        if (j>=0x20 && j<=0x2F)                 // check relative branches
          {                                     // calculate absolute address
          if (inbuf[i] < 0x80) l = i+1 + (int)inbuf[i];
          else l = i+128 + (int)inbuf[i] | 0xFF00;
          printf("%04X", (WORD)l);
          }
        else                                    
          {
          if (j>=0xA0 && j<=0xAF)
            printf("%02X", (UCHAR)inbuf[i]);    // immediate addressing mode
          else
            printf("%03X", (UCHAR)inbuf[i]);    // direct addressing mode
          }
        if (j>=0xE0 && j<=0xEF) printf(",x");   // indexed 1 byte offset
        if (j>=0x60 && j<=0x6F) printf(",x");   // indexed 1 byte offset 

        // For the sake of legibility:
        // Insert a blank line after unconditional BRAs and JMPs
        if (!StrCmpNI(&mnemonic6805[j].mneStr[5],"jmp",3)) printf("\n");
        if (!StrCmpNI(&mnemonic6805[j].mneStr[5],"bra",3)) printf("\n");
        } // end else "\t"

      // One odd last byte left - indeterminable instruction
      else printf("\t\t\t---\t\t\t; FCB  '%c'", (UCHAR)inbuf[i]);
      } // end if mnemonic6805[j].mneStr[0]

    // 1 byte instruction
    else
      {
      printf ("\t\t");
      printf (mnemonic6805[j].mneStr);

      // For the sake of legibility:
      // Insert a blank line after unconditional RTS, RTI and JMPs
      if (!StrCmpNI(&mnemonic6805[j].mneStr[4], "rts",3)) printf("\n");
      if (!StrCmpNI(&mnemonic6805[j].mneStr[4], "rti",3)) printf("\n");
      if (!StrCmpNI(&mnemonic6805[j].mneStr[4], "jmp",3)) printf("\n");
      }

    if (!StrCmpNI(&mnemonic6805[j].mneStr[2], "---", 3))
      {
      if ((UCHAR)inbuf[i] >= SPACE && (UCHAR)inbuf[i] < 0x7F)
        printf("\t\t\t; FCB  '%c'", (UCHAR)inbuf[i]);
      else
        printf("\t\t\t; FCB  $%02X", (UCHAR)inbuf[i]);
      }

    printf("\n");
    i++; k++;
    } // end while */

  printf("\n"); 
  if (bytesrd > ROMSIZE)
    printf("Warning: %s exceeds M68HC05 ROM-Size\n", argv_name);
  
  printf("%d Source lines produced\n", k);
  close(fh);
  exit(0);
  } // main

//****************************************************************************
//  --DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//
char* _pszfile_ = __FILE__; // Global pointer to current __FILE__

//----------------------------------------------------------------------------
//
//                          DebugStop
//
//  Usage example:  DebugStop("Function()", testNr);
//
// //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
// _pszfile_ =__FILE__;
// printf("pc=%04X, _pc1=%04X\n", pc, _pc1);
// DebugStop("p2org()", 5);
// //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//
void DebugStop(char* _info, int num)
  {
  while (_kbhit() != 0) _getch();   // flush key-buffer 

//ha//  if (_debugSwPass == 1) printf("PASS1 [%03d--%s]", num, _info);
//ha//  else if (_debugSwPass == 2) printf("PASS2 [%03d--%s]", num, _info);
  printf(" %s -- press <ESC> for exit --\n\n", PathFindFileName(_pszfile_));
  if (_getch() == ESC) exit(0);
  } // DebugStop

//----------------------------------------------------------------------------
//
//                          DebugPrintBuffer
//
//  Usage example:  DebugPrintBuffer(buffer, count);
//
// //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
// _pszfile_ =__FILE__;
// if (_debugSwPass == 2)
// {
// printf("[002a--p1equ()]\n");
// printf("pc=%04X  errSymbol_ptr[0]=%02X\n", pc, errSymbol_ptr[0]);
// printf("errSymbol_ptr ");    
// DebugPrintBuffer(errSymbol_ptr, SYMLEN);
// }
// //--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//
void DebugPrintBuffer(char *buf, int count)
  {
  int i;

  printf("Buf: ");
  // print as ASCII-HEX 00 010: ...
  for (i=0; i<count; i++) printf("%02X ", (unsigned char)buf[i]);
  // print as UNICODE characters: "..."
  printf("\n\x22");
  for (i=0; i<count; i++)
    {
    // print 0s as SPACE
    if (buf[i] == 0) printf("%c", SPACE); 
    else printf("%c", (unsigned char)buf[i]);
    }
  printf("\x22\n");       // "

//ha//  if (_debugSwPass == 1) printf("PASS1:");
//ha//  else if (_debugSwPass == 2) printf("PASS2:");
  printf(" %s -- press <ESC> for exit --\n\n", PathFindFileName(_pszfile_));
  if (_getch() == ESC) exit(0);
  } // DebugPrintBuffer

//----------------------------------------------------------------------------

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//_pszfile_ =__FILE__;
//ha//printf("[003--main()]\n");
//ha//printf("mnemonic6805[%02X].mneStr=%s\n", (UCHAR)j, mnemonic6805[j].mneStr);
//ha//printf("mnemonic6805[%02X].mneStr[0]=%02X ['\t'=%02X}\n", 
//ha//                 (UCHAR)j, mnemonic6805[(UCHAR)j].mneStr[0], '\t');
//ha//printf("&inbuf[%d] ", i);
//ha//DebugPrintBuffer(&inbuf[i], 10);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//_pszfile_ =__FILE__;
//ha//printf("[004--main()]\n");
//ha//printf("mnemonic6805[%02X].mneStr=%s\n", (UCHAR)j, &mnemonic6805[(UCHAR)(j)].mneStr[6]);
//ha//printf("mnemonic6805[%02X].mneStr[0]=%02X\n", (UCHAR)j, &mnemonic6805[(UCHAR)(j)].mneStr[6]);
//ha//printf("&inbuf[%d] ", i);
//ha//DebugPrintBuffer(&inbuf[i], 10);
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//ha//_pszfile_ =__FILE__;
//ha//if (i > bytesrd-4)
//ha//{
//ha//printf("i=%d   bytesrd=%d", i, bytesrd);
//ha//DebugStop("main()", 22);
//ha//}
//ha////--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--

//  --DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--DEBUG--
//****************************************************************************

//--------------------------end-of-c++-module-----------------------------------
