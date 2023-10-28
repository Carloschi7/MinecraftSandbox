#pragma once
#include <memory>
#include "Texture.h"
#include "Utils.h"

class Window;
class Camera;
class Shader;
class VertexManager;
class FrameBuffer;

namespace GlCore
{
	struct State
	{
		State();
		~State();

		//Global game-handled state instance, should call this for global configuration
		static State& GlobalInstance()
		{
			static State state;
			return state;
		}

		Window* game_window;
		Camera* camera;
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
}