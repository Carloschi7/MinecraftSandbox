#pragma once
#include <memory>
#include "MainIncl.h"
#include "GameDefinitions.h"

//Preferred singleton implementation
namespace GlCore
{
	//Uniform names definitions
	const std::string g_ModelUniformName = "model";
	const std::string g_EntitySelectedUniformName = "entity_selected";
	const std::string g_DiffuseTextureUniformName = "diffuse_texture";
	const std::string g_ViewUniformName = "view";
	const std::string g_ProjUniformName = "proj";
	const std::string g_SkyboxUniformName = "skybox";
	const glm::mat4 g_NullMatrix{};

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
			const GameDefs::DrawableData& exp_norms);

	private:
		Renderer();

		void IRender(const glm::mat4& model,
			const VertexManager& vm,
			Shader& shd);

		void IRenderVisible(const glm::mat4& model,
			const VertexManager& vm,
			Shader& shd,
			const GameDefs::DrawableData& exp_norms);
	};
}