#pragma once
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

	static Renderer* GetInstance();
	static void Render(const glm::mat4* model,
		std::shared_ptr<VertexManager> vm,
		std::shared_ptr<Shader> shd,
		const std::vector<RendererTextureRef>& tex_ref);

private:
	Renderer();

	void IRender(const glm::mat4* model,
		std::shared_ptr<VertexManager> vm,
		std::shared_ptr<Shader> shd,
		const std::vector<RendererTextureRef>& tex_ref);
};