#include <iostream>
#include "../application/Application.h"
#include "Memory.h"

int main()
{
	//mem::InitializeArena(1024 * 1024 * 1024);
	Application MinecraftClone;
	MinecraftClone.OnUserCreate();
	MinecraftClone.OnUserRun();
	//mem::DestroyArena();

	return 0;
}
