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

	glm::vec3 g_FramebufferPlayerOffset = glm::vec3(0.0f, 50.0f, 0.0f);
	glm::mat4 g_DepthSpaceMatrix(1.0f);

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

	void Renderer::Render(Shader* shd, const VertexManager& vm, CubeMap* cubemap, const glm::mat4& model)
	{
		GetInstance().IRender(shd, vm, cubemap, model);
	}

	void Renderer::RenderInstanced(u32 count)
	{
		GetInstance().IRenderInstanced(count);
	}

	void Renderer::IRender(Shader* shd, const VertexManager& vm, CubeMap* cubemap, const glm::mat4& model)
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
	void Renderer::IRenderInstanced(u32 count)
	{
		//This function is used only to draw block meshes in this application
		//so leave the static interval (0, 36)
		glDrawArraysInstanced(GL_TRIANGLES, 0, 36, count);
	}

	void DispatchBlockRendering(glm::vec3*& position_buf, u32*& texture_buf, u32& count)
	{
		auto block_vm = pstate->block_vm;

		pstate->block_shader->Use();
		block_vm->BindVertexArray();

		//Unmap buffers for rendering and then remapping them
		block_vm->UnmapAttributePointer(0);
		block_vm->UnmapAttributePointer(1);
		GlCore::Renderer::RenderInstanced(count);
		position_buf = static_cast<glm::vec3*>(block_vm->InstancedAttributePointer(0));
		texture_buf = static_cast<u32*>(block_vm->InstancedAttributePointer(1));

		count = 0;
	}
	void DispatchDepthRendering(glm::vec3*& position_buf, u32& count)
	{
		auto depth_vm = pstate->depth_vm;

		pstate->depth_shader->Use();
		depth_vm->BindVertexArray();

		depth_vm->UnmapAttributePointer(0);
		GlCore::Renderer::RenderInstanced(count);
		position_buf = static_cast<glm::vec3*>(depth_vm->InstancedAttributePointer(0));
		
		count = 0;
	}
}
