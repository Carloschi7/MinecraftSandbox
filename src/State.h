#pragma once
#include <memory>

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
		void SetBlockShader(std::shared_ptr<Shader> block_shader_);
		void SetDepthShader(std::shared_ptr<Shader> depth_shader_);
		void SetWaterShader(std::shared_ptr<Shader> water_shader_);
		void SetBlockVM(std::shared_ptr<VertexManager> vertex_manager_);
		void SetDepthVM(std::shared_ptr<VertexManager> vertex_manager_);
		void SetWaterVM(std::shared_ptr<VertexManager> vertex_manager_);
		void SetShadowFramebuffer(std::shared_ptr<FrameBuffer> shadow_framebuffer_);

		Window& GameWindow();
		Camera& GameCamera();
		std::shared_ptr<Shader> BlockShader();
		std::shared_ptr<Shader> DepthShader();
		std::shared_ptr<Shader> WaterShader();
		std::shared_ptr<VertexManager> BlockVM();
		std::shared_ptr<VertexManager> DepthVM();
		std::shared_ptr<VertexManager> WaterVM();
		std::shared_ptr<FrameBuffer> DepthFramebuffer();
	private:
		State();
		~State();

		Window* game_window;
		Camera* camera;
		std::shared_ptr<Shader> block_shader;
		std::shared_ptr<Shader> depth_shader;
		std::shared_ptr<Shader> water_shader;
		std::shared_ptr<VertexManager> block_vm;
		std::shared_ptr<VertexManager> depth_vm;
		std::shared_ptr<VertexManager> water_vm;
		std::shared_ptr<FrameBuffer> shadow_framebuffer;
	};
}