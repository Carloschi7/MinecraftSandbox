#include <chrono>
#include <glm/gtc/type_ptr.hpp>

#include "Renderer.h"
#include "Vertices.h"
#include "GlStructure.h"
#include "Entrypoint.h"

namespace GlCore
{
	std::atomic_bool g_LogicThreadShouldRun = true;
	std::atomic_bool g_SerializationRunning = false;
	std::map<std::string, std::thread::id> g_ThreadPool;

	glm::vec3 g_FramebufferPlayerOffset = glm::vec3(0.0f, 50.0f, 0.0f);
	glm::mat4 g_DepthSpaceMatrix(1.0f);

	uint32_t g_Drawcalls = 0;

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

	void Renderer::Render(std::shared_ptr<Shader> shd, const VertexManager& vm, std::shared_ptr<CubeMap> cubemap, const glm::mat4& model)
	{
		GetInstance().IRender(shd, vm, cubemap, model);
	}

	void Renderer::RenderInstanced(uint32_t count)
	{
		GetInstance().IRenderInstanced(count);
	}

	void Renderer::WaitForAsyncGpu(const std::vector<GLsync>& fences)
	{
		GetInstance().IWaitForAsyncGpu(fences);
	}

	void Renderer::IRender(std::shared_ptr<Shader> shd, const VertexManager& vm, std::shared_ptr<CubeMap> cubemap, const glm::mat4& model)
	{
		shd->Use();
		vm.BindVertexArray();

		if (cubemap)
		{
			cubemap->BindTexture();
			shd->Uniform1i(0, "skybox");
		}

		//If we uniform something the shader gets bound automatically
		if (model != g_NullMatrix)
		{
			shd->UniformMat4f(model, "model");
		}
		glDrawArrays(GL_TRIANGLES, 0, vm.GetIndicesCount());
	}
	void Renderer::IRenderInstanced(uint32_t count)
	{
		glDrawArraysInstanced(GL_TRIANGLES, 0, 36, count);
	}

	void Renderer::IWaitForAsyncGpu(const std::vector<GLsync>& fences)
	{
		for (uint32_t i = 0; i < fences.size(); i++)
		{
			GLenum waitResult = GL_UNSIGNALED;
			while (waitResult != GL_ALREADY_SIGNALED && waitResult != GL_CONDITION_SATISFIED) {
				waitResult = glClientWaitSync(fences[i], GL_SYNC_FLUSH_COMMANDS_BIT, 1);
			}
		}
	}


	void DispatchBlockRendering(glm::vec3* position_buf, uint32_t* texture_buf, uint32_t& count)
	{
		State& state = State::GetState();
		auto block_vm = state.BlockVM();

		state.BlockShader()->Use();
		block_vm->BindVertexArray();
		block_vm->EditInstance(0, position_buf, sizeof(glm::vec3) * count, 0);
		block_vm->EditInstance(1, texture_buf, sizeof(uint32_t) * count, 0);

		GlCore::Renderer::RenderInstanced(count);
		g_Drawcalls++;

		count = 0;
	}
	void DispatchDepthRendering(glm::vec3* position_buf, uint32_t& count)
	{
		State& state = State::GetState();
		auto depth_vm = state.DepthVM();

		state.DepthShader()->Use();
		depth_vm->BindVertexArray();
		depth_vm->EditInstance(0, position_buf, sizeof(glm::vec3) * count, 0);

		GlCore::Renderer::RenderInstanced(count);
		g_Drawcalls++;

		count = 0;
	}
}
