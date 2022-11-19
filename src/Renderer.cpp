#include <chrono>
#include "Renderer.h"
#include "Vertices.h"
#include "GlStructure.h"
#include "Entrypoint.h"

GlCore::Root::RootImpl GlCore::Root::m_InternalPayload;

namespace GlCore
{
	std::atomic_bool g_LogicThreadShouldRun = true;

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

	void Renderer::IRender(const RendererPayload& pl)
	{
		if (pl.block_selected)
			pl.shd->Uniform1i(true, g_EntitySelectedUniformName);

		pl.shd->Use();
		pl.vm->BindVertexArray();

		//Texture handling(only max one can be defined)
		if (pl.texture && pl.cubemap)
			throw std::runtime_error("Too many definitions");

		if (pl.texture)
		{
			pl.texture->Bind();
			pl.shd->Uniform1i(0, g_DiffuseTextureUniformName);
		}
		if (pl.cubemap)
		{
			pl.cubemap->BindTexture();
			pl.shd->Uniform1i(0, g_SkyboxUniformName);
		}


		//If we uniform something the shader gets bound automatically
		if (pl.model != g_NullMatrix)
			pl.shd->UniformMat4f(pl.model, g_ModelUniformName);

		if (pl.dd)
		{
			for (uint32_t i = 0; i < pl.dd->second; i++)
				glDrawArrays(GL_TRIANGLES, pl.dd->first[i], 6);
		}
		else
		{
			glDrawArrays(GL_TRIANGLES, 0, pl.vm->GetIndicesCount());
		}

		if (pl.block_selected)
			pl.shd->Uniform1i(false, g_EntitySelectedUniformName);
	}
}
