#include "chip8.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#define max(a, b) (((a) > (b)) ? (a) : (b))

#define DEBUG
#define CHIP8_LOG "chip8.log"
	FILE *clog = NULL;
#define touch_log()\
	do {\
		clog = fopen(CHIP8_LOG, "a");\
		assert(clog != NULL);\
		fprintf(clog, "[ %time% ]\n");\
	} while (0)
#define perr(...)\
	do {\
		if (!clog) touch_log();\
		fprintf(clog, "ERROR: " __VA_ARGS__);\
		close_log(clog);\
		exit(1);\
	} while (0)
#define err_unkown_opcode(op) perr("Unknown opcode 0x%X\n", op)
#define err_file_oversize(file) perr("Unable to load %s due to oversize\n", file)
#ifdef DEBUG
#define plog(...)\
	do {\
		if (!clog) touch_log();\
		fprintf(clog, __VA_ARGS__);\
	} while (0)
#else
#define plog(...)
#endif

typedef struct {
	uint8_t x, y, n, kk;
	uint16_t nnn;
	Chip8* c8;
} Paras;
typedef void (*Chip8_Op)(Paras*);

uint8_t chip8_fontset[80] =
{
	//0
	0b11110000,
	0b10010000,
	0b10010000,
	0b10010000,
	0b11110000,
	//1
	0b00100000,
	0b01100000,
	0b00100000,
	0b00100000,
	0b01110000,
	//2
	0b11110000,
	0b00010000,
	0b11110000,
	0b10000000,
	0b11110000,
	//3
	0b11110000,
	0b00010000,
	0b11110000,
	0b00010000,
	0b11110000,
	//4
	0b10010000,
	0b10010000,
	0b11110000,
	0b00010000,
	0b00010000,
	//5
	0b11110000,
	0b10000000,
	0b11110000,
	0b00010000,
	0b11110000,
	//6
	0b11110000,
	0b10000000,
	0b11110000,
	0b10010000,
	0b11110000,
	//7
	0b11110000,
	0b00010000,
	0b00100000,
	0b01000000,
	0b01000000,
	//8
	0b11110000,
	0b10010000,
	0b11110000,
	0b10010000,
	0b11110000,
	//9
	0b11110000,
	0b10010000,
	0b11110000,
	0b00010000,
	0b11110000,
	//A
	0b01100000,
	0b10010000,
	0b11110000,
	0b10010000,
	0b10010000,
	//B
	0b11100000,
	0b10010000,
	0b11100000,
	0b10010000,
	0b11100000,
	//C
	0b01110000,
	0b10000000,
	0b10000000,
	0b10000000,
	0b01110000,
	//D
	0b11100000,
	0b10010000,
	0b10010000,
	0b10010000,
	0b11100000,
	//E
	0b11110000,
	0b10000000,
	0b11110000,
	0b10000000,
	0b11110000,
	//F
	0b11110000,
	0b10000000,
	0b11110000,
	0b10000000,
	0b10000000,
};

void chip8_initialize(Chip8 *c8)
{
	c8->pc = 0x200; //Program counter starts at 0x200
	c8->opcode = 0; //Reset current opcode
	c8->I = 0;		//Reset index register
	c8->sp = 0;		//Reset stack pointer

	//Clear display
	memset(c8->gfx, 0, 64 * 32);
	//Clear stack
	memset(c8->stack, 0, 16);
	//Clear register V0-VF
	memset(c8->V, 0, 16);
	//Clear memory
	memset(c8->memory, 0, 4096);
	//Clear key state
	memset(c8->key, 0, 16);
	//Reset timers
	c8->delay_timer = 0;
	c8->sound_timer = 0;
	//Reset draw flag
	c8->drawFlag = 1;
	//Load fontset
	memcpy(c8->memory, chip8_fontset, 80);

	//srand()
}

void chip8_loadgame(Chip8 *c8, const char* name, const char *bin, size_t size)
{
	if (size > 0x1000 - 0x200) err_file_oversize(name);
	memcpy(c8->memory + 0x200, bin, size);
	plog("Load file %s\n", name);
}

void emulatecycle(Chip8 *c8)
{
	static Chip8_Op opcodes[] = {
		op00ez, op1nnn, op2nnn, op3xkk,
		op4xkk, op5xy0, op6xkk, op7xkk,
		op8xyz, op9xy0, opannn, opbnnn,
		opcxkk, opdxyn, opexzz, opfxzz,
	};

	Paras p = {
		(c8->opcode & 0x0F00) >> 8,
		(c8->opcode & 0x00F0) >> 4,
		c8->opcode & 0x000F,
		c8->opcode & 0x00FF,
		c8->opcode & 0x0FFF,
		c8,
	};

	c8->opcode = (c8->memory[c8->pc] << 8) | c8->memory[c8->pc + 1];

	plog("%3X %4X ", c8->pc, c8->opcode);

	opcodes[(c8->opcode & 0xF000) >> 12](&p);

	timer_tick(c8);
}

void op00ez(Paras *p)
{
	switch (p->n) {
		case 0x0: op00e0(p); break;
		case 0xe: op00ee(p); break;
		default: err_unkown_opcode(p->c8->opcode);
	}
}

void op00e0(Paras *p)
{
	plog("Clear the display\n");
	memset(p->c8->gfx, 0, 64 * 32);
	p->c8->drawFlag = 1;
	p->c8->pc += 2;
}

void op00ee(Paras *p)
{
	plog("Return from a subroutine\n");
	p->c8->pc = p->c8->stack[--p->c8->sp];
}

void op1nnn(Paras *p)
{
	plog("Jump to location %3X\n", p->nnn);
	p->c8->pc = p->nnn;
}

void op2nnn(Paras *p)
{
	plog("Call subroutine at %3X\n", p->nnn);
	p->c8->stack[p->c8->sp++] = p->c8->pc + 2;
	p->c8->pc = p->nnn;
}

void op3xkk(Paras *p)
{
	plog("Skip next instruction if V%X == V%X\n", p->x, p->y);
	p->c8->pc += (p->c8->V[p->x] == p->kk) ? 4 : 2;
}

void op4xkk(Paras *p)
{
	plog("Skip next instruction if V%X != V%X\n", p->x, p->y);
	p->c8->pc += (p->c8->V[p->x] != p->kk) ? 4 : 2;
}

void op5xy0(Paras *p)
{
	plog("Skip next instruction if V%X == V%X\n", p->x, p->y);
	p->c8->pc += (p->c8->V[p->x] == p->c8->V[p->y]) ? 4 : 2;
}

void op6xkk(Paras *p)
{
	plog("Set V%X = V%X\n", p->x, p->y);
	p->c8->V[p->x] = p->kk;
	p->c8->pc += 2;
}

void op7xkk(Paras *p)
{
	plog("Set V%X = V%X + %d\n", p->x, p->x, p->kk);
	p->c8->V[p->x] += p->kk;
	p->c8->pc += 2;
}

void op8xyz(Paras *p)
{
	static Chip8_Op opcodes[] = {
		op8xy0, op8xy1, op8xy2, op8xy3,
		op8xy4, op8xy5, op8xy6, op8xy7,
	};
	if (0 <= p->n && p->n < 8)
		opcodes[p->n](p);
	else if (p->n == 0xe)
		op8xye(p);
	else
		err_unkown_opcode(p->c8->opcode);
}

void op8xy0(Paras *p)
{
	plog("Set V%X = V%X\n", p->x, p->y);
	p->c8->V[p->x] = p->c8->V[p->y];
	p->c8->pc += 2;
}

void op8xy1(Paras *p)
{
	plog("Set V%X = V%X OR V%X\n", p->x, p->x, p->y);
	p->c8->V[p->x] |= p->c8->V[p->y];
	p->c8->pc += 2;
}

void op8xy2(Paras *p)
{
	plog("Set V%X = V%X AND V%X\n", p->x, p->x, p->y);
	p->c8->V[p->x] &= p->c8->V[p->y];
	p->c8->pc += 2;
}

void op8xy3(Paras *p)
{
	plog("Set V%X = V%X XOR V%X\n", p->x, p->x, p->y);
	p->c8->V[p->x] ^= p->c8->V[p->y];
	p->c8->pc += 2;
}

void op8xy4(Paras *p)
{
	plog("Set V%X = V%X + V%X\n", p->x, p->x, p->y);
	p->c8->V[0xF] = (p->c8->V[p->x] > 0xFF - p->c8->V[p->y]) ? 1 : 0;
	p->c8->V[p->x] += p->c8->V[p->y];
	p->c8->pc += 2;
}

void op8xy5(Paras *p)
{
	plog("Set V%X = V%X - V%X, set VF = NOT borrow\n", p->x, p->x, p->y);
	p->c8->V[0xF] = (p->c8->V[p->x] > p->c8->V[p->y]) ? 1 : 0;
	p->c8->V[p->x] -= p->c8->V[p->y];
	p->c8->pc += 2;
}

void op8xy6(Paras *p)
{
	plog("Set V%X = V%X SHR 1, set VF\n", p->x, p->x);
	p->c8->V[0xF] = p->c8->V[p->x] & 0x01;
	p->c8->V[p->x] >>= 1;
	p->c8->pc += 2;
}

void op8xy7(Paras *p)
{
	plog("Set V%X = V%X - V%X, set VF = NOT borrow\n", p->x, p->y, p->x);
	p->c8->V[0xF] = (p->c8->V[p->y] > p->c8->V[p->x]) ? 1 : 0;
	p->c8->V[p->x] = p->c8->V[p->y] - p->c8->V[p->x];
	p->c8->pc += 2;
}

void op8xye(Paras *p)
{
	plog("Set V%X = V%X SHL 1, set VF\n", p->x, p->x);
	p->c8->V[0xF] = (p->c8->V[p->x] >> 7) & 0x01;
	p->c8->V[p->x] <<= 1;
	p->c8->pc += 2;
}

void op9xy0(Paras *p)
{
	plog("Skip next instruction if V%X != V%X\n", p->x, p->y);
	p->c8->pc += (p->c8->V[p->x] != p->c8->V[p->y]) ? 4 : 2;
}

void opannn(Paras *p)
{
	plog("Set I = %3X\n", p->nnn);
	p->c8->I = p->nnn;
	p->c8->pc += 2;
}

void opbnnn(Paras *p)
{
	plog("Jump to location %3X + V0", p->nnn);
	p->c8->pc = p->c8->V[0] + p->nnn;
}

void opcxkk(Paras *p)
{
	plog("Set V%X = random byte AND %2X", p->x, p->kk);
	p->c8->V[p->x] = (rand() % 256) & p->kk;
	p->c8->pc += 2;
}

void opdxyn(Paras *p)
{
	plog("Display sprite\n");
	p->c8->V[0xF] = 0;
	for (int yline = 0; yline < p->n; ++yline) {
		uint16_t pixel = p->c8->memory[p->c8->I + yline];
		for (int xline = 0; xline < 8; ++xline) {
			if ((pixel & (0x80 >> xline)) != 0) {
				if (p->c8->gfx[p->x + xline + (p->y + yline) * 64] == 1)
					p->c8->V[0xF] = 1;
				p->c8->gfx[p->x + xline + (p->y + yline) * 64] ^= 1;
			}
		}
	}
	p->c8->drawFlag = 1;
	p->c8->pc += 2;
}

void opexzz(Paras *p)
{
	switch (p->kk) {
		case 0x9e: opex9e(p); break;
		case 0xa1: opexa1(p); break;
		default: err_unkown_opcode(p->c8->opcode);
	}
}

void opex9e(Paras *p)
{
	plog("Skip next instruction if V%X key is pressed\n", p->x);
	p->c8->pc += p->c8->key[p->c8->V[p->x]] ? 4 : 2;
}

void opexa1(Paras *p)
{
	plog("Skip next instruction if V%X key is not pressed\n", p->x);
	p->c8->pc += !(p->c8->key[p->c8->V[p->x]]) ? 4 : 2;
}

void opfxzz(Paras *p)
{
	//			07, 0a, 15, 18, 1e, 29, 33, 55, 65
	//%18		7,  10, 3,  6,  12, 5,  15, 13, 11
	static Chip8_Op opcodes[] = {
		NULL, NULL, NULL, opfx15,
		NULL, opfx29, opfx18, opfx07,
		NULL, NULL, opfx0a, opfx65, NULL,
		opfx55, NULL, opfx33, NULL, NULL, NULL,
	}; int addr = p->kk % 18;
	if (!opcodes[addr]) err_unkown_opcode(p->c8->opcode);
	opcodes[addr](p);
}

void opfx07(Paras *p)
{
	plog("Set V%X = delay timer value\n", p->x);
	p->c8->V[p->x] = p->c8->delay_timer;
	p->c8->pc += 2;
}

void opfx0a(Paras *p)
{
	plog("Wait for a key press, store the key value in V%X\n", p->x);
	for (int i = 0; i < 16; ++i) {
		if (p->c8->key[i]) {
			p->c8->V[p->x] = i;
			p->c8->pc += 2;
			break;
		}
	}
}

void opfx15(Paras *p)
{
	plog("Set delay timer = V%X\n", p->x);
	p->c8->delay_timer = p->c8->V[p->x];
	p->c8->pc += 2;
}

void opfx18(Paras *p)
{
	plog("Set sound timer = V%X\n", p->x);
	p->c8->sound_timer = p->c8->V[p->x];
	p->c8->pc += 2;
}

void opfx1e(Paras *p)
{
	plog("Set I = I + V%X\n", p->x);
	p->c8->V[0xF] = (p->c8->I > 0xFFF - p->c8->V[p->x]) ? 1 : 0;
	p->c8->I += p->c8->V[p->x];
	p->c8->pc += 2;
}

void opfx29(Paras *p)
{
	plog("Set I = location of sprite for digit V%X\n", p->x);
	p->c8->I = p->c8->V[p->x] * 5;
	p->c8->pc += 2;
}

void opfx33(Paras *p)
{
	plog("Store BCD of V%X in memory locations I...I+2\n", p->x);
	p->c8->memory[p->c8->I] = p->c8->V[p->x] / 100;
	p->c8->memory[p->c8->I + 1] = p->c8->V[p->x] / 10 % 10;
	p->c8->memory[p->c8->I + 2] = p->c8->V[p->x] % 10;
	p->c8->pc += 2;
}

void opfx55(Paras *p)
{
	plog("Stores V0 to V%X in memory starting at I\n", p->x);
	for (int i = 0; i <= p->x; ++i) {
		p->c8->memory[p->c8->I + i] = p->c8->V[i];
	}
	p->c8->I += p->x + 1;
}

void opfx65(Paras *p)
{
	plog("Fills V0 to V%X with values from memory starting at I\n", p->x);
	for (int i = 0; i <= p->x; ++i) {
		p->c8->V[i] = p->c8->memory[p->c8->I + i];
	}
	p->c8->I += p->x + 1;
}

void timer_tick(Chip8* c8)
{
	c8->delay_timer = max(c8->delay_timer - 1, 0);
	c8->sound_timer = max(c8->sound_timer - 1, 0);
}