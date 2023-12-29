#include "State.h"

#include "VertexManager.h"
#include "Window.h"
#include "Camera.h"
#include "Shader.h"
#include "FrameBuffer.h"
#include "Memory.h"

namespace GlCore
{


	State::~State()
	{
		//Free all the objects
		Memory::Arena* arena = pstate->memory_arena;
		Memory::DeleteUnchecked(arena, cubemap_shader);
		Memory::DeleteUnchecked(arena, block_shader);
		Memory::DeleteUnchecked(arena, depth_shader);
		Memory::DeleteUnchecked(arena, water_shader);
		Memory::DeleteUnchecked(arena, inventory_shader);
		Memory::DeleteUnchecked(arena, crossaim_shader);
		Memory::DeleteUnchecked(arena, drop_shader);
		Memory::DeleteUnchecked(arena, screen_shader);

		Memory::DeleteUnchecked(arena, block_vm);
		Memory::DeleteUnchecked(arena, drop_vm);
		Memory::DeleteUnchecked(arena, depth_vm);
		Memory::DeleteUnchecked(arena, water_vm);
		Memory::DeleteUnchecked(arena, inventory_vm);
		Memory::DeleteUnchecked(arena, inventory_entry_vm);
		Memory::DeleteUnchecked(arena, crossaim_vm);
		Memory::DeleteUnchecked(arena, decal2d_vm);
		Memory::DeleteUnchecked(arena, screen_vm);

		Memory::DeleteUnchecked(arena, screen_framebuffer);
		Memory::DeleteUnchecked(arena, shadow_framebuffer);
		
		Memory::DeleteUnchecked(arena, cubemap);
	}

	State* pstate = nullptr;
}

