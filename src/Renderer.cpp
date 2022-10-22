#include "Renderer.h"
#include "Vertices.h"

namespace GlCore
{

	Renderer::Renderer()
	{
	}

	Renderer& Renderer::GetInstance()
	{
		static Renderer instance;
		return instance;
	}

	void Renderer::Render(const glm::mat4& model,
		const VertexManager& vm,
		Shader& shd)
	{
		GetInstance().IRender(model, vm, shd);
	}

	void Renderer::RenderVisible(const glm::mat4& model,
		const VertexManager& vm,
		Shader& shd,
		const GameDefs::DrawableData& exp_norms)
	{
		GetInstance().IRenderVisible(model, vm, shd, exp_norms);
	}

	void Renderer::IRender(const glm::mat4& model,
		const VertexManager& vm,
		Shader& shd)
	{
		shd.Use();
		vm.BindVertexArray();

		//If we uniform something the shader gets bound automatically
		if (model != g_NullMatrix)
			shd.UniformMat4f(model, g_ModelUniformName);

		glDrawArrays(GL_TRIANGLES, 0, vm.GetIndicesCount());
	}

	void Renderer::IRenderVisible(const glm::mat4& model,
		const VertexManager& vm,
		Shader& shd,
		const GameDefs::DrawableData& exp_norms)
	{
		shd.Use();
		vm.BindVertexArray();

		if (model != g_NullMatrix)
			shd.UniformMat4f(model, g_ModelUniformName);

		for (uint32_t i = 0; i < exp_norms.second; i++)
			glDrawArrays(GL_TRIANGLES, exp_norms.first[i], 6);
	}
}
