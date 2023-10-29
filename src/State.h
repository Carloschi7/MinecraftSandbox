#pragma once
#include <memory>
#include <vector>
#include "Texture.h"

class Window;
class Camera;
class Shader;
class VertexManager;
class FrameBuffer;
namespace mem {
	class MemoryArena;
}

namespace GlCore
{
	struct State
	{
		State();
		~State();

		Window* game_window;
		Camera* camera;
		mem::MemoryArena* memory_arena;

		std::vector<Texture> game_textures;
		std::shared_ptr<Shader> cubemap_shader;
		std::shared_ptr<Shader> block_shader;
		std::shared_ptr<Shader> depth_shader;
		std::shared_ptr<Shader> water_shader;
		std::shared_ptr<Shader> inventory_shader;
		std::shared_ptr<Shader> crossaim_shader;
		std::shared_ptr<Shader> drop_shader;
		std::shared_ptr<VertexManager> block_vm;
		std::shared_ptr<VertexManager> drop_vm;
		std::shared_ptr<VertexManager> depth_vm;
		std::shared_ptr<VertexManager> water_vm;
		std::shared_ptr<VertexManager> inventory_vm;
		std::shared_ptr<VertexManager> inventory_entry_vm;
		std::shared_ptr<VertexManager> crossaim_vm;
		std::shared_ptr<FrameBuffer> shadow_framebuffer;
		std::shared_ptr<CubeMap> cubemap;
	};

	extern State* pstate;
}