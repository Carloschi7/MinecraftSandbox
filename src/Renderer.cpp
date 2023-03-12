#include <chrono>
#include <glm/gtc/type_ptr.hpp>

#include "Renderer.h"
#include "Vertices.h"
#include "GlStructure.h"
#include "Entrypoint.h"

GlCore::Root::RootImpl GlCore::Root::m_InternalPayload;

namespace GlCore
{
	std::atomic_bool g_LogicThreadShouldRun = true;
	std::atomic_bool g_SerializationRunning = false;
	std::map<std::string, std::thread::id> g_ThreadPool;

	glm::vec3 g_FramebufferPlayerOffset = glm::vec3(0.0f, 50.0f, 0.0f);
	glm::mat4 g_DepthSpaceMatrix(1.0f);

	//Root

	void Root::SetGameWindow(Window* game_window_)
	{
		m_InternalPayload.game_window = game_window_;
	}
	void Root::SetGameCamera(Camera* game_camera_)
	{
		m_InternalPayload.camera = game_camera_;
	}
	void Root::SetBlockShader(std::shared_ptr<Shader> block_shader_)
	{
		m_InternalPayload.block_shader = block_shader_;
	}
	void Root::SetDepthShader(std::shared_ptr<Shader> depth_shader_)
	{
		m_InternalPayload.depth_shader = depth_shader_;
	}
	void Root::SetBlockVM(std::shared_ptr<VertexManager> vertex_manager_)
	{
		m_InternalPayload.block_vm = vertex_manager_;
	}
	void Root::SetDepthVM(std::shared_ptr<VertexManager> vertex_manager_)
	{
		m_InternalPayload.depth_vm = vertex_manager_;
	}
	void Root::SetShadowFramebuffer(std::shared_ptr<FrameBuffer> shadow_framebuffer_)
	{
		m_InternalPayload.shadow_framebuffer = shadow_framebuffer_;
	}
	Window& Root::GameWindow()
	{
		return *m_InternalPayload.game_window;
	}
	Camera& Root::GameCamera()
	{
		return *m_InternalPayload.camera;
	}

	std::shared_ptr<Shader> Root::BlockShader()
	{
		return m_InternalPayload.block_shader;
	}

	std::shared_ptr<Shader> Root::DepthShader()
	{
		return m_InternalPayload.depth_shader;
	}

	std::shared_ptr<VertexManager> Root::BlockVM()
	{
		return m_InternalPayload.block_vm;
	}

	std::shared_ptr<VertexManager> Root::DepthVM()
	{
		return m_InternalPayload.depth_vm;
	}

	std::shared_ptr<FrameBuffer> Root::DepthFramebuffer()
	{
		return m_InternalPayload.shadow_framebuffer;
	}
	
	Root::Root()
	{
	}
	Root::~Root()
	{
	}

	//RenderQueue
	

	Renderer::Renderer()
	{
	}

	Renderer::~Renderer()
	{
	}

	void Renderer::Init()
	{
		GetInstance();
	}

	Renderer& Renderer::GetInstance()
	{
		static Renderer instance;
		return instance;
	}

	void Renderer::Render(const RendererPayload& pl)
	{
		GetInstance().IRender(pl);
	}

	void Renderer::RenderInstanced(uint32_t count)
	{
		GetInstance().IRenderInstanced(count);
	}

	void Renderer::IRender(const RendererPayload& pl)
	{
		pl.shd->Use();
		pl.vm->BindVertexArray();

		//Texture handling(only max one can be defined)
		if (pl.texture && pl.cubemap)
			throw std::runtime_error("Too many definitions");

		if (pl.texture)
		{
			pl.texture->Bind();
			pl.shd->Uniform1i(0, "texture_diffuse");
		}
		if (pl.cubemap)
		{
			pl.cubemap->BindTexture();
			pl.shd->Uniform1i(0, "skybox");
		}


		//If we uniform something the shader gets bound automatically
		if (pl.model != g_NullMatrix)
		{
			pl.shd->UniformMat4f(pl.model, "model");
		}

		if (pl.dd)
		{
			for (uint32_t i = 0; i < pl.dd->second; i++)
				glDrawArrays(GL_TRIANGLES, pl.dd->first[i], 6);
		}
		else
		{
			glDrawArrays(GL_TRIANGLES, 0, pl.vm->GetIndicesCount());
		}
	}
	void Renderer::IRenderInstanced(uint32_t count)
	{
		glDrawArraysInstanced(GL_TRIANGLES, 0, 36, count);
	}
}
