#include <iostream>
#include "../application/Application.h"
#include "Memory.h"

int main()
{
	//We will use 1Gib of ram for this program
	mem::InitializeArena(1024 * 1024 * 1024);
	Application MinecraftClone;
	MinecraftClone.OnUserCreate();
	MinecraftClone.OnUserRun();
	//Check for memory leaks
#ifdef _DEBUG
	MC_ASSERT(mem::g_Arena.memory_used == mem::unfreed_mem,
		"There are some memory leaks during application shutdown, please fix them, as they may leak some data outside the arena");
#endif
	mem::DestroyArena();

	return 0;
}
