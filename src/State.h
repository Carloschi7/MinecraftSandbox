#pragma once
#include <memory>
#include "Texture.h"

class Window;
class Camera;
class Shader;
class VertexManager;
class FrameBuffer;

namespace GlCore
{
	class State
	{
	public:
		static State& GetState()
		{
			static State state;
			return state;
		}

		void SetGameWindow(Window* game_window_);
		void SetGameCamera(Camera* game_camera_);

		void SetCubemapShader(std::shared_ptr<Shader> cubemap_shader_);
		void SetBlockShader(std::shared_ptr<Shader> block_shader_);
		void SetDepthShader(std::shared_ptr<Shader> depth_shader_);
		void SetWaterShader(std::shared_ptr<Shader> water_shader_);
		void SetInventoryShader(std::shared_ptr<Shader> inventory_shader_);
		void SetCrossaimShader(std::shared_ptr<Shader> crossaim_shader_);
		void SetDropShader(std::shared_ptr<Shader> drop_shader_);

		void SetBlockVM(std::shared_ptr<VertexManager> vertex_manager_);
		void SetDropVM(std::shared_ptr<VertexManager> vertex_manager_);
		void SetDepthVM(std::shared_ptr<VertexManager> vertex_manager_);
		void SetWaterVM(std::shared_ptr<VertexManager> vertex_manager_);
		void SetInventoryVM(std::shared_ptr<VertexManager> vertex_manager_);
		void SetInventoryEntryVM(std::shared_ptr<VertexManager> vertex_manager_);
		void SetCrossaimVM(std::shared_ptr<VertexManager> vertex_manager_);

		void SetShadowFramebuffer(std::shared_ptr<FrameBuffer> shadow_framebuffer_);
		void SetCubemap(std::shared_ptr<CubeMap> cubemap_);

		Window& GameWindow();
		Camera& GameCamera();
		std::shared_ptr<Shader> CubemapShader();
		std::shared_ptr<Shader> BlockShader();
		std::shared_ptr<Shader> DepthShader();
		std::shared_ptr<Shader> WaterShader();
		std::shared_ptr<Shader> InventoryShader();
		std::shared_ptr<Shader> CrossaimShader();
		std::shared_ptr<Shader> DropShader();

		std::shared_ptr<VertexManager> BlockVM();
		std::shared_ptr<VertexManager> DropVM();
		std::shared_ptr<VertexManager> DepthVM();
		std::shared_ptr<VertexManager> WaterVM();
		std::shared_ptr<VertexManager> InventoryVM();
		std::shared_ptr<VertexManager> InventoryEntryVM();
		std::shared_ptr<VertexManager> CrossaimVM();

		std::shared_ptr<FrameBuffer> DepthFramebuffer();
		std::shared_ptr<CubeMap> Cubemap();
	private:
		State();
		~State();

		Window* game_window;
		Camera* camera;
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