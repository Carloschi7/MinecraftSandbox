#pragma once
#include <memory>
#include <list>
#include <thread>
#include <mutex>
#include "MainIncl.h"
#include "GameDefinitions.h"
#include "Vertices.h"
#include "Utils.h"

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
	const glm::mat4 g_IdentityMatrix{ 1.0f };
	static constexpr bool g_MultithreadedRendering = true;
	extern std::atomic_bool g_LogicThreadShouldRun;
	//Basic normals
	static constexpr glm::vec3 g_PosX{	1.0f,	0.0f,	0.0f };
	static constexpr glm::vec3 g_NegX{ -1.0f,	0.0f,	0.0f };
	static constexpr glm::vec3 g_PosY{	0.0f,	1.0f,	0.0f };
	static constexpr glm::vec3 g_NegY{	0.0f,  -1.0f,	0.0f };
	static constexpr glm::vec3 g_PosZ{	0.0f,	0.0f,	1.0f };
	static constexpr glm::vec3 g_NegZ{	0.0f,	0.0f,  -1.0f };

	//Static handler of the most used game system entities,
	//Gets initialised by the user with the Init static function
	class Root
	{
	public:
		static void SetGameWindow(Window* game_window_);
		static void SetGameCamera(Camera* game_camera_);
		static void SetBlockShader(std::shared_ptr<Shader> block_shader_);
		static void SetBlockVM(std::shared_ptr<VertexManager> vertex_manager_);
		static Window& GameWindow();
		static Camera& GameCamera();
		static std::shared_ptr<Shader> BlockShader();
		static std::shared_ptr<VertexManager> BlockVM();
	private:
		Root();
		~Root();
		struct RootImpl
		{
			Window* game_window;
			Camera* camera;
			std::shared_ptr<Shader> block_shader;
			std::shared_ptr<VertexManager> block_vm;
		};
		static RootImpl m_InternalPayload;
	};

	struct RendererPayload
	{
		glm::mat4 model;
		const VertexManager* vm;
		Shader* shd;
		const Texture* texture;
		CubeMap* cubemap;
		const GlCore::DrawableData* dd;
		const bool block_selected;
	};

	class Renderer
	{
		class Fetcher;
	public:
		static void Init();
		static Renderer& GetInstance();
		static void Render(const RendererPayload& p);
	private:
		Renderer();
		~Renderer();
		void IRender(const RendererPayload& p);
	};
}

//Defining types related to multithreading
template<class T>
using VecType = typename Utils::ConditionalVector<T, GlCore::g_MultithreadedRendering>;
template<class T>
using ListType = typename Utils::ConditionalList<T, GlCore::g_MultithreadedRendering>;