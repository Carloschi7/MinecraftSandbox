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
		const std::vector<glm::vec3>& exp_norms)
	{
		GetInstance().IRenderVisible(model, vm, shd, exp_norms);
	}

	void Renderer::IRender(const glm::mat4& model,
		const VertexManager& vm,
		Shader& shd)
	{
		vm.BindVertexArray();
		shd.Use();

		//If the matrix is different from the null matrix
		if (model != g_NullMatrix)
			shd.UniformMat4f(model, g_ModelUniformName);

		glDrawArrays(GL_TRIANGLES, 0, vm.GetIndicesCount());
	}

	void Renderer::IRenderVisible(const glm::mat4& model,
		const VertexManager& vm,
		Shader& shd,
		const std::vector<glm::vec3>& exp_norms)
	{
		vm.BindVertexArray();
		shd.Use();

		if (model != g_NullMatrix)
			shd.UniformMat4f(model, g_ModelUniformName);

		for (const auto& vec : exp_norms)
			glDrawArrays(GL_TRIANGLES, Utils::GetNormVertexBegin(vec), 6);
	}
}
