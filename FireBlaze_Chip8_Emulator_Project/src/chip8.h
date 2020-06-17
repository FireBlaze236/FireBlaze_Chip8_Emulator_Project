#pragma once


#include "SDL.h"


class Chip8 {

public:
	SDL_Window* window;
	SDL_Surface* surface;
	SDL_Renderer* renderer;
	unsigned int mul = 10;
	unsigned short opcode;
    unsigned char memory[4096]; // 4096 bytes
	unsigned char V[16]; // 16 x 8 bit
	unsigned short I; // 16 bit
	unsigned short pc; // 16 bit

	unsigned char gfx[64*32]; //screen

	unsigned char delay_timer;
	unsigned char sound_timer;

	unsigned short stack[16];
	unsigned short sp;

	unsigned char key[16];
	bool running;
	
public:
	bool drawFlag;
	Chip8();
	~Chip8();
	void Init();
	void LoadProgram(const char* filename);
	//opcode functions
	void SetKeyStates();
	void EmulateCycle();
	void ClearScreen();
	void DrawGraphics();
	unsigned char GetKey();

};