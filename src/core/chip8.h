#ifndef _CHIP8_H_
#define _CHIP8_H_
#include <stdint.h>

typedef struct
{
	uint8_t memory[4096]; //4K memory
	uint8_t V[16];		  //15 general purpose registers + 1 "carry flag"
	uint8_t gfx[64 * 32]; //framebuffer
	uint8_t key[16];
	uint8_t delay_timer;
	uint8_t sound_timer;
	uint8_t drawFlag;
	uint16_t I;		 //index registers
	uint16_t pc;	 //program counter
	uint16_t opcode; //current opcode
	uint16_t stack[16];
	uint16_t sp; //stack pointer
} Chip8;

void chip8_initialize(Chip8*);
void chip8_loadgame(Chip8*, const char*, const char*, size_t);
void chip8_emulatecycle(Chip8*);

void close_log();
#endif