#include "State.h"

#include "VertexManager.h"
#include "Window.h"
#include "Camera.h"
#include "Shader.h"
#include "FrameBuffer.h"

namespace GlCore
{
	State::State() :
		camera(nullptr), game_window(nullptr)
	{
	}
	State::~State()
	{
	}

	void State::SetGameWindow(Window* game_window_)
	{
		game_window = game_window_;
	}
	void State::SetGameCamera(Camera* game_camera_)
	{
		camera = game_camera_;
	}
	void State::SetCubemapShader(std::shared_ptr<Shader> cubemap_shader_)
	{
		cubemap_shader = cubemap_shader_;
	}
	void State::SetBlockShader(std::shared_ptr<Shader> block_shader_)
	{
		block_shader = block_shader_;
	}
	void State::SetDepthShader(std::shared_ptr<Shader> depth_shader_)
	{
		depth_shader = depth_shader_;
	}
	void State::SetWaterShader(std::shared_ptr<Shader> water_shader_)
	{
		water_shader = water_shader_;
	}
	void State::SetInventoryShader(std::shared_ptr<Shader> inventory_shader_)
	{
		inventory_shader = inventory_shader_;
	}
	void State::SetCrossaimShader(std::shared_ptr<Shader> crossaim_shader_)
	{
		crossaim_shader = crossaim_shader_;
	}
	void State::SetDropShader(std::shared_ptr<Shader> drop_shader_)
	{
		drop_shader = drop_shader_;
	}
	void State::SetBlockVM(std::shared_ptr<VertexManager> vertex_manager_)
	{
		block_vm = vertex_manager_;
	}
	void State::SetDropVM(std::shared_ptr<VertexManager> vertex_manager_)
	{
		drop_vm = vertex_manager_;
	}
	void State::SetDepthVM(std::shared_ptr<VertexManager> vertex_manager_)
	{
		depth_vm = vertex_manager_;
	}
	void State::SetWaterVM(std::shared_ptr<VertexManager> vertex_manager_)
	{
		water_vm = vertex_manager_;
	}
	void State::SetInventoryVM(std::shared_ptr<VertexManager> vertex_manager_)
	{
		inventory_vm = vertex_manager_;
	}
	void State::SetInventoryEntryVM(std::shared_ptr<VertexManager> vertex_manager_)
	{
		inventory_entry_vm = vertex_manager_;
	}
	void State::SetCrossaimVM(std::shared_ptr<VertexManager> vertex_manager_)
	{
		crossaim_vm = vertex_manager_;
	}
	void State::SetShadowFramebuffer(std::shared_ptr<FrameBuffer> shadow_framebuffer_)
	{
		shadow_framebuffer = shadow_framebuffer_;
	}

	void State::SetCubemap(std::shared_ptr<CubeMap> cubemap_)
	{
		cubemap = cubemap_;
	}


	Window& State::GameWindow()
	{
		return *game_window;
	}
	Camera& State::GameCamera()
	{
		return *camera;
	}

	std::shared_ptr<Shader> State::CubemapShader()
	{
		return cubemap_shader;
	}

	std::shared_ptr<Shader> State::BlockShader()
	{
		return block_shader;
	}

	std::shared_ptr<Shader> State::DepthShader()
	{
		return depth_shader;
	}
	std::shared_ptr<Shader> State::WaterShader()
	{
		return water_shader;
	}

	std::shared_ptr<Shader> State::InventoryShader()
	{
		return inventory_shader;
	}

	std::shared_ptr<Shader> State::CrossaimShader()
	{
		return crossaim_shader;
	}

	std::shared_ptr<Shader> State::DropShader()
	{
		return drop_shader;
	}

	std::shared_ptr<VertexManager> State::BlockVM()
	{
		return block_vm;
	}

	std::shared_ptr<VertexManager> State::DropVM()
	{
		return drop_vm;
	}

	std::shared_ptr<VertexManager> State::DepthVM()
	{
		return depth_vm;
	}

	std::shared_ptr<VertexManager> State::WaterVM()
	{
		return water_vm;
	}

	std::shared_ptr<VertexManager> State::InventoryVM()
	{
		return inventory_vm;
	}

	std::shared_ptr<VertexManager> State::InventoryEntryVM()
	{
		return inventory_entry_vm;
	}

	std::shared_ptr<VertexManager> State::CrossaimVM()
	{
		return crossaim_vm;
	}

	std::shared_ptr<FrameBuffer> State::DepthFramebuffer()
	{
		return shadow_framebuffer;
	}

	std::shared_ptr<CubeMap> State::Cubemap()
	{
		return cubemap;
	}


}

