#include <iostream>
#include "chip8.h"

int main(int argc, char* argv[])
{
	Chip8 myChip8;
	myChip8.Init();
	myChip8.LoadProgram("res/tetris.rom");
	//myChip8.LoadProgram("res/file1.chip8");
	while (myChip8.running)
	{
		myChip8.EmulateCycle();
		myChip8.SetKeyStates();
		myChip8.DrawGraphics();
		SDL_Delay(1);
	}
	return 0;
}