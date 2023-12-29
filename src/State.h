#pragma once
#include <memory>
#include <vector>
#include "Texture.h"
#include "Vertices.h"

class Window;
class Camera;
class Shader;
class VertexManager;
class FrameBuffer;
namespace Memory {
	class Arena;
}

namespace GlCore
{


	struct State
	{
		State() = default;
		State(const State&) = delete;
		State& operator=(const State&) = delete;
		~State();

		Window*					game_window;
		Camera*					camera;
		Memory::Arena*			memory_arena;
		MeshStorage*			mesh_storage;
		std::vector<Texture>	game_textures;

		Shader*					cubemap_shader;	
		Shader*					block_shader;	
		Shader*					depth_shader;	
		Shader*					water_shader;	
		Shader*					inventory_shader;
		Shader*					crossaim_shader;
		Shader*					drop_shader;
		Shader*					screen_shader;
		VertexManager*			block_vm;	
		VertexManager*			drop_vm;
		VertexManager*			depth_vm;
		VertexManager*			water_vm;			
		VertexManager*			inventory_vm;		
		VertexManager*			inventory_entry_vm;	
		VertexManager*			crossaim_vm;		
		//This is used to render 2d sprites when the player holds them
		VertexManager*			decal2d_vm;
		VertexManager*			screen_vm;

		//Framebuffer on which all the scene drawcalls will be executed
		FrameBuffer*			screen_framebuffer;
		FrameBuffer*			shadow_framebuffer;	
		
		CubeMap*				cubemap;				
	};

	extern State* pstate;
}