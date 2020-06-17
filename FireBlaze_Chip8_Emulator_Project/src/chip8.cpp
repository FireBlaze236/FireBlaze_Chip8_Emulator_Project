#include "chip8.h"
#include <fstream>
#include <iostream>
#include <random>
#include <time.h>


static unsigned char chip8_fontset[80] =
{
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};


Chip8::Chip8()
{
	SDL_Init(SDL_INIT_EVERYTHING);
	window = SDL_CreateWindow("Chip8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 64 * mul, 32 * mul, SDL_WINDOW_SHOWN);
	surface = SDL_GetWindowSurface(window);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	running = true;
}

Chip8::~Chip8()
{
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void Chip8::Init()
{
	opcode = I =  0;
	sp = 0;
	pc = 0x200;

	for (int i = 0; i < 16;i++)
	{
		V[i] = 0;
		stack[i] = 0;
		key[i] = 0;
	}
		
	//Load font-set

	for (int i = 0;i < 80;i++)
		memory[i] = chip8_fontset[i];


	delay_timer = sound_timer = 0;

}

void Chip8::EmulateCycle()
{
	opcode = ( (memory[pc] << 8) | memory[pc + 1] );
	//std::cout << std::hex << opcode << std::endl;
	switch (opcode & 0xF000)
	{
	case 0x0000:
		switch (opcode & 0x00FF)
		{
		case 0x00E0: // Clear the screen
			drawFlag = true;
			ClearScreen();
			pc += 2;
			break;
		case 0x00EE: //Return from jump
			sp--;
			pc = stack[sp];
			break;
		default:
			std::cout << "Invalid Opcode" << std::endl;
			break;
		}
		break;

	case 0x1000: //Jump to location nnn
		pc = opcode & 0x0FFF;
		break;

	case 0x2000: //Call subroutine
		stack[sp] = pc+2;
		sp++;
		pc = opcode & 0x0FFF;
		break;

	case 0x3000: // Vx == kk then skip
		if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
			pc += 4;
		else
			pc += 2;
		break;

	case 0x4000: //Vx != kk then skip
		if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
			pc += 4;
		else
			pc += 2;
		break;

	case 0x5000: //Vx == Vy then skip
		if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
			pc += 4;
		else
			pc += 2;
		break;

	case 0x6000: //Vx = kk
		V[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF);
		pc += 2;
		break;

	case 0x7000: //Vx = Vx + kk
		V[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
		pc += 2;
		break;

	case 0x8000: 
		
		switch (opcode & 0x000F)
		{
			
		case 0x0000: //Vx = Vy
			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;

		case 0x0001: //OR
			V[(opcode & 0x0F00) >> 8] =(V[(opcode & 0x0F00) >> 8] | V[(opcode & 0x00F0) >> 4]);
			pc += 2;
			break;

		case 0x0002: //AND
			V[(opcode & 0x0F00) >> 8] = (V[(opcode & 0x0F00) >> 8] & V[(opcode & 0x00F0) >> 4]);
			pc += 2;
			break;

		case 0x0003: //XOR
			V[(opcode & 0x0F00) >> 8] = (V[(opcode & 0x0F00) >> 8] ^ V[(opcode & 0x00F0) >> 4]);
			pc += 2;
			break;

		case 0x0004: //Vx + Vy ADD and Carry VF
			static unsigned short result = (V[(opcode & 0x0F00) >> 8] + V[(opcode & 0x00F0) >> 4]);
			if (result > 255)
				V[0xF] = 1;
			else
				V[0xF] = 0;

			V[(opcode & 0x0F00) >> 8] = result & 0x00FF;
			pc += 2;
			break;

		case 0x0005: // Vx-Vy SUB and carry VF
			if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4])
				V[0xF] = 1;
			else
				V[0xF] = 0;

			V[(opcode & 0x0F00) >> 8] = (V[(opcode & 0x0F00) >> 8] - V[(opcode & 0x00F0) >> 4]);
			pc += 2;
			break;

		case 0x0006: //SHR
			V[0xF] = V[(opcode & 0x00F0) >> 4] & 1;
			V[(opcode & 0x00F0) >> 4] = V[(opcode & 0x00F0) >> 4] >> 1;
			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
			pc += 2;
			break;

		case 0x0007: //Vy - Vx Sub and carry VF
			if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8])
				V[0xF] = 1;
			else
				V[0xF] = 0;

			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4] - V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;

		case 0x000E: //SHL
			V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x8000;
			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] << 1;
			V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;

		default:
			std::cout << "Invalid opcode!" << std::endl;
			running = false;
			break;
		}
	break;

	case 0x9000: // Vx != Vy skip
		if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
			pc += 4;
		else
			pc += 2;
		break;

	case 0xA000: // I = nn
		I = opcode & 0x0FFF;
		pc += 2;
		break;

	case 0xB000: //Jump to nn+v0
		stack[sp] = pc + 2;
		sp++;
		pc = (opcode & 0x0FFF) + V[0];
		break;
		
	case 0xC000: //random & nn
	{	unsigned short x = (opcode & 0x0F00) >> 8;
		V[x] = (rand() % 255) & (opcode & 0x00FF);
		pc += 2;
	}
	break;

	case 0xD000: //opcode dxyn

	{

		unsigned short x = V[(opcode & 0x0F00) >> 8];

		unsigned short y = V[(opcode & 0x00F0) >> 4];

		unsigned short height = opcode & 0x000F;

		unsigned short pixel;

		V[0xF] = 0;

		for (int yline = 0; yline < height; yline++) {

			pixel = memory[I + yline];

			for (int xline = 0; xline < 8; xline++) {

				if ((pixel & (0x80 >> xline)) != 0) {

					if (gfx[(x + xline + ((y + yline) * 64))] == 1)

						V[0xF] = 1;

					gfx[x + xline + ((y + yline) * 64)] ^= 1;

				}

			}

		}

		drawFlag = true;

		pc += 2;

	}

	break;

	case 0xE000: //Check key states
		switch (opcode & 0x00FF)
		{
		case 0x009E: //Pressed
			if (key[V[(opcode & 0x0F00) >> 8]] == 1)
				pc += 4;
			else
				pc += 2;
			break;
		case 0x00A1: // Not pressed
			if (key[V[(opcode & 0x0F00) >> 8]] != 1)
				pc += 4;
			else
				pc += 2;
			break;
		default:
			std::cout << "Invalid opcode!" << std::endl;
			running = false;
			break;
		}
		break;

	case 0xF000:
		switch (opcode & 0x00FF)
		{
		case 0x0007: // Vx = delay
			V[(opcode & 0x0F00) >> 8] = delay_timer;
			pc += 2;
			break;

		case 0x000A: // Halt execution and wait for key press
			V[(opcode & 0x0F00) >> 8] = GetKey();
			pc += 2;
			break;

		case 0x0015: //Delay = Vx
			delay_timer = V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;

		case 0x0018: //Sound = Vx
			sound_timer = V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;

		case 0x001E: // I = I + Vx
			I += V[(opcode & 0x0F00) >> 8];
			pc += 2;
			break;

		case 0x0029: //I = number(Vx) sprite location
			I = 5 * (V[(opcode & 0x0F00) >> 8]);
			pc += 2;
			break;

		case 0x0033: //BCD code
		{
			unsigned char n = V[(opcode & 0x0F00) >> 8];
			memory[I + 2] = n % 10;
			n = n / 10;
			memory[I + 1] = n % 10;
			n = n / 10;
			memory[I] = n;
			pc += 2;
		}
			break;


		case 0x0055: // load memory from V0 to Vx starting from mem[I]
		{
			unsigned short x = (opcode & 0x0F00) >> 8;
			int j = I;
			for (int i = 0; i <= x; i++)
			{
				memory[j] = V[i];
				j++;
			}
			I = I + x + 1;

			pc += 2;
		}
			break;
		case 0x0065: // load registers from V0 to Vx starting from mem[I]
		{
			unsigned short x = (opcode & 0x0F00) >> 8;
			int j = I;
			for (int i = 0; i <= x; i++)
			{
				V[i] = memory[j];
				j++;
			}
			I = I + x + 1;
			pc += 2;
		}
			break;
		default:
			std::cout << "Invalid opcode!" << std::endl;
			running = false;
			break;


		}
		break;
	default:
		std::cout << "Invalid opcode!" << std::endl;
		running = false;
		break;
			 
	}

}


void Chip8::ClearScreen()
{
	for (int i = 0; i < 2048; i++)
	{
		gfx[i] = 0;
	}
}


void Chip8::LoadProgram(const char* filename)
{
	std::ifstream f;
	f.open(filename, std::ios::binary);

	
	std::cout << "Loading file : " << filename << std::endl;

	if (!f)
	{
		std::cout << "File load error." << std::endl;
		f.close();
		return;
	}

	if (f)
	{
		f.seekg(0, f.end);
		int length = f.tellg();
		f.seekg(0, f.beg);


		char* buffer = new char[length];
		f.read(buffer, length);




		if (f)
		{
			std::cout << "File loaded!" << std::endl;
		}
		else
		{
			std::cout << "File load error!" << std::endl;
		}

		f.close();

		for (int i = 0; i < length; i++)
		{
			memory[i + 0x200] = buffer[i];
		}

		std::cout << "Program loaded to memory." << std::endl;

		delete[] buffer;
	}
}


unsigned char Chip8::GetKey()
{
	SDL_Event e;
	SDL_WaitEvent(&e);
	if (e.type != SDL_EventType::SDL_KEYDOWN) SDL_WaitEvent(&e);
	unsigned char k=0;
	switch (e.key.keysym.sym)
	{
	case SDL_KeyCode::SDLK_1:
		k = 0;
		break;
	case SDL_KeyCode::SDLK_2:
		k = 1;
		break;
	case SDL_KeyCode::SDLK_3:
		k = 2;
		break;
	case SDL_KeyCode::SDLK_4:
		k = 3;
		break;
	case SDL_KeyCode::SDLK_q:
		k = 4;
		break;
	case SDL_KeyCode::SDLK_w:
		k = 5;
		break;
	case SDL_KeyCode::SDLK_e:
		k = 6;
		break;
	case SDL_KeyCode::SDLK_r:
		k = 7;
		break;
	case SDL_KeyCode::SDLK_a:
		k = 8;
		break;
	case SDL_KeyCode::SDLK_s:
		k = 9;
		break;
	case SDL_KeyCode::SDLK_d:
		k = 10;
		break;
	case SDL_KeyCode::SDLK_f:
		k = 11;
		break;
	case SDL_KeyCode::SDLK_z:
		k = 12;
		break;
	case SDL_KeyCode::SDLK_x:
		k = 13;
		break;
	case SDL_KeyCode::SDLK_c:
		k = 14;
		break;
	case SDL_KeyCode::SDLK_v:
		k = 15;
		break;
	}
	return k;
}

void Chip8::DrawGraphics()
{
	//If the draw flag is set, update the screen

	if (drawFlag) {

		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);

		SDL_RenderClear(renderer);

		int x = 0;

		int y = 0;

		for (int i = 0; i < 64 * 32; ++i) {

			if (i % 64 == 0) {

				y += 9;

				x = 0;

			}

			if (gfx[i] == 1) {

				SDL_Rect fillRect = { x, y, 10, 10 };

				SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF);

				SDL_RenderFillRect(renderer, &fillRect);

			}

			x += 9;

		}

		SDL_RenderPresent(renderer);

		drawFlag = false;

	}

}

void Chip8::SetKeyStates()
{
	SDL_Event e;
	SDL_PollEvent(&e);
	if (e.type == SDL_EventType::SDL_KEYDOWN)
	{
		if (e.key.keysym.sym == SDLK_1)
			key[0] = 1;
		else
			key[0] = 0;

		if (e.key.keysym.sym == SDLK_2)
			key[1] = 1;
		else
			key[1] = 0;

		if (e.key.keysym.sym == SDLK_3)
			key[2] = 1;
		else
			key[2] = 0;



		if (e.key.keysym.sym == SDLK_4)
			key[3] = 1;
		else
			key[3] = 0;
		//qwer
		if (e.key.keysym.sym == SDLK_q)
			key[4] = 1;
		else
			key[4] = 0;

		if (e.key.keysym.sym == SDLK_w)
			key[5] = 1;
		else
			key[5] = 0;

		if (e.key.keysym.sym == SDLK_e)
			key[6] = 1;
		else
			key[6] = 0;

		if (e.key.keysym.sym == SDLK_r)
			key[7] = 1;
		else
			key[7] = 0;

		//r3

		if (e.key.keysym.sym == SDLK_a)
			key[8] = 1;
		else
			key[8] = 0;

		if (e.key.keysym.sym == SDLK_s)
			key[9] = 1;
		else
			key[9] = 0;

		if (e.key.keysym.sym == SDLK_d)
			key[10] = 1;
		else
			key[10] = 0;

		if (e.key.keysym.sym == SDLK_f)
			key[11] = 1;
		else
			key[11] = 0;

		// r4

		if (e.key.keysym.sym == SDLK_z)
			key[12] = 1;
		else
			key[12] = 0;

		if (e.key.keysym.sym == SDLK_x)
			key[13] = 1;
		else
			key[13] = 0;

		if (e.key.keysym.sym == SDLK_c)
			key[14] = 1;
		else
			key[14] = 0;

		if (e.key.keysym.sym == SDLK_v)
			key[15] = 1;
		else
			key[15] = 0;
		if (e.key.keysym.sym == SDLK_ESCAPE)
			running = false;

	}
	
}