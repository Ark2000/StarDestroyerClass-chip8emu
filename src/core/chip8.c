#include "chip8.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


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

bool chip8_drawFlag(Chip8 *c8)
{
	return c8->drawFlag;
}

void chip8_emulateCycle(Chip8 *c8)
{
	//Fetch opcode
	uint8_t x, y, n;
	uint8_t kk;
	uint16_t nnn;
	c8->opcode = c8->memory[c8->pc] << 8 | c8->memory[c8->pc + 1];
	x = (c8->opcode & 0x0F00) >> 8;
	y = (c8->opcode & 0x00F0) >> 4;
	n = c8->opcode & 0x000F;
	kk = c8->opcode & 0x00FF;
	nnn = c8->opcode & 0x0FFF;

	//Decode opcode
	switch (c8->opcode & 0xF000) {
	case 0x0000:
		switch (n) {
		case 0x0: //CLS
			memset(c8->gfx, 0, 64 * 32);
			c8->drawFlag = 1;
			c8->pc += 2;
			break;
		case 0xE://RET
			c8->pc = c8->stack[--c8->sp];
			break;
		default:
			err_unkown_opcode(c8->opcode);
		}
		break;
	case 0x1000://JP addr
		c8->pc = nnn;
		break;
	case 0x2000: //Call subroutine at NNN
		c8->stack[c8->sp++] = c8->pc + 2;
		c8->pc = nnn;
		break;
	case 0x3000://SE Vx, byte
		c8->pc += (c8->V[x] == kk) ? 4 : 2;
		break;
	case 0x4000://SNE Vx,byte
		c8->pc += (c8->V[x] != kk) ? 4 : 2;
		break;
	case 0x5000://SE Vx,Vy
		c8->pc += (c8->V[x] == c8->V[y]) ? 4 : 2;
		break;
	case 0x6000://LD Vx,byte
		c8->V[x] = kk;
		c8->pc += 2;
		break;
	case 0x7000://ADD Vx,byte
		c8->V[x] += kk;
		c8->pc += 2;
		break;
	case 0x8000: {
		switch (n) {
		case 0x0://LD Vx, Vy
			c8->V[x] = c8->V[y];
			break;
		case 0x1://OR Vx, Vy
			c8->V[x] |= c8->V[y];
			break;
		case 0x2://AND Vx, Vy
			c8->V[x] &= c8->V[y];
			break;
		case 0x3://XOR Vx, Vy
			c8->V[x] ^= c8->V[y];
			break;
		case 0x4://ADD Vx, Vy
			c8->V[0xF] = (c8->V[x] > 0xFF - c8->V[y]) ? 1 : 0;
			c8->V[x] += c8->V[y];
			break;
		case 0x5://SUB Vx, Vy
			c8->V[0xF] = (c8->V[x] > c8->V[y]) ? 1 : 0;
			c8->V[x] -= c8->V[y];
			break;
		case 0x6://SHR Vx {,Vy}
			c8->V[0xF] = c8->V[x] & 0x01;
			c8->V[x] >>= 1;
			break;
		case 0x7://SUBN Vx, Vy
			c8->V[0xF] = (c8->V[y] > c8->V[x]) ? 1 : 0;
			c8->V[x] = c8->V[y] - c8->V[x];
			break;
		case 0xE://SHL Vx {, Vy}
			c8->V[0xF] = (c8->V[x] >> 7) & 0x01;
			c8->V[x] <<= 1;
			break;
		default:
			err_unkown_opcode(c8->opcode);
		}
		c8->pc += 2;
		break;
	case 0x9000: //SNE Vx, Vy
		c8->pc += (c8->V[x] != c8->V[y]) ? 4 : 2;
		break;
	case 0xA000: //LD I, addr
		c8->I = nnn;
		c8->pc += 2;
		break;
	case 0xB000://JP V0, addr
		c8->pc = c8->V[0] + nnn;
		break;
	case 0xC000://RND Vx, byte
		c8->V[x] = (rand() % 256) & kk;
		c8->pc += 2;
		break;
	case 0xD000://DRW Vx, Vy, nibble
		uint16_t height = n;
		uint16_t pixel;
		c8->V[0xF] = 0;
		for (int yline = 0; yline < height; ++yline) {
			pixel = c8->memory[c8->I + yline];
			for (int xline = 0; xline < 8; ++xline) {
				if ((pixel & (0x80 >> xline)) != 0) {
					if (c8->gfx[x + xline + (y + yline) * 64] == 1)
						c8->V[0xF] = 1;
					c8->gfx[x + xline +(y + yline) * 64] ^= 1;
				}
			}
		}
		c8->drawFlag = 1;
		c8->pc += 2;
		break;
	case 0xE000:
		switch (n) {
		case 0xE://SKP Vx
			c8->pc += c8->key[c8->V[x]] ? 4 : 2;
			break;
		case 0x1://SKNP Vx
			c8->pc += !(c8->key[c8->V[x]]) ? 4 : 2;
			break;
		default:
			err_unkown_opcode(c8->opcode);
		}
		break;
	case 0xF000: {
		switch (kk) {
		case 0x07://LD Vx, DT
			c8->V[x] = c8->delay_timer;
			break;
		case 0x0A://LD Vx, K
			for (int k = 0; k < 16; ++k) {
				if (c8->key[k]) {
					c8->V[x] = k;
					c8->pc += 2;
					break;
				}
			}
			c8->pc -= 2;
			break;
		case 0x0015://LD DT,Vx
			c8->delay_timer = c8->V[x];
			break;
		case 0x0018://LD ST, Vx
			c8->sound_timer = c8->V[x];
			break;
		case 0x001E://ADD I, Vx
			c8->V[0xF] = (c8->I > 0xFFF - c8->V[x]) ? 1 : 0;
			c8->I += c8->V[x];
			break;
		case 0x0029://LD F, Vx
			c8->I = c8->V[x] * 5;
			break;
		case 0x0033://LD B, Vx
			c8->memory[c8->I] = c8->V[x] / 100;
			c8->memory[c8->I + 1] = c8->V[x] / 10 % 10;
			c8->memory[c8->I + 2] = c8->V[x] % 10;
			break;
		case 0x0055://LD Vx, [I]
			for (int i = 0; i <= x; ++i) {
				c8->memory[c8->I + i] = c8->V[i];
			}
			c8->I += x + 1;
			break;
		case 0x0065://LD Vx, [I]
			for (int i = 0; i <= x; ++i) {
				c8->V[i] = c8->memory[c8->I + i];
			}
			c8->I += x + 1;
			break;
		default:
			err_unkown_opcode(c8->opcode);
		}
		c8->pc += 2;
		break;
	}
	default:
		fprintf(stderr, "Unkown opcode 0x%X\n", c8->opcode);
	}

	//Update timers
	if (c8->delay_timer > 0) 
		--c8->delay_timer;
	if (c8->sound_timer > 0) {
		if (c8->sound_timer == 1)
			printf("BEEP!\n");
		--c8->sound_timer;
	}
}

void close_log()
{
	fclose(clog);
}