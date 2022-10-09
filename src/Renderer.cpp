#include "Renderer.h"

Renderer* Renderer::GetInstance()
{
	static Renderer* instance;
	return instance;
}

void Renderer::Render(const glm::mat4* model,
	std::shared_ptr<VertexManager> vm,
	std::shared_ptr<Shader> shd,
	const std::vector<RendererTextureRef>& tex_ref)
{
	GetInstance()->IRender(model, vm, shd, tex_ref);
}

void Renderer::IRender(const glm::mat4* model,
	std::shared_ptr<VertexManager> vm,
	std::shared_ptr<Shader> shd,
	const std::vector<RendererTextureRef>& tex_ref)
{
	vm->BindVertexArray();
	shd->Use();

	for (uint32_t i = 0; i < tex_ref.size(); i++)
	{
		tex_ref[i].tex->Bind(i);
		shd->Uniform1i(i, tex_ref[i].tex_uniform_name);
	}

	if(model)
		shd->UniformMat4f(*model, "model");
	
	glDrawArrays(GL_TRIANGLES, 0, vm->GetIndicesCount());
}
