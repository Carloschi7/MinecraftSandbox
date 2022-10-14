#pragma once
#include <memory>
#include "MainIncl.h"

struct RendererTextureRef
{
	Texture* tex;
	std::string tex_uniform_name;
};

//Preferred singleton implementation
class Renderer
{
public:
	static Renderer& GetInstance();
	static void Render(const glm::mat4& model,
		const VertexManager& vm,
		Shader& shd);

	static void RenderVisible(const glm::mat4& model,
		const VertexManager& vm,
		Shader& shd,
		const std::vector<glm::vec3>& exp_norms);

private:
	Renderer();

	void IRender(const glm::mat4& model,
		const VertexManager& vm,
		Shader& shd);

	void IRenderVisible(const glm::mat4& model,
		const VertexManager& vm,
		Shader& shd,
		const std::vector<glm::vec3>& exp_norms);
};
