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
	const glm::mat4 g_NullMatrix{};
	const glm::mat4 g_IdentityMatrix{ 1.0f };
	static constexpr bool g_MultithreadedRendering = MC_MULTITHREADING;
	static constexpr uint32_t g_MaxInstancedObjs = 500;
	static constexpr uint32_t g_DepthMapWidth = 1024;
	static constexpr uint32_t g_DepthMapHeight = 1024;
	extern std::atomic_bool g_LogicThreadShouldRun;
	//Basic normals
	static constexpr glm::vec3 g_PosX{	1.0f,	0.0f,	0.0f };
	static constexpr glm::vec3 g_NegX{ -1.0f,	0.0f,	0.0f };
	static constexpr glm::vec3 g_PosY{	0.0f,	1.0f,	0.0f };
	static constexpr glm::vec3 g_NegY{	0.0f,  -1.0f,	0.0f };
	static constexpr glm::vec3 g_PosZ{	0.0f,	0.0f,	1.0f };
	static constexpr glm::vec3 g_NegZ{	0.0f,	0.0f,  -1.0f };
	//Framebuffer data
	extern glm::vec3 g_FramebufferPlayerOffset;
	extern glm::mat4 g_DepthSpaceMatrix;
	//Large data buffers
	extern glm::vec3* g_DynamicPositionBuffer;
	extern uint32_t* g_DynamicTextureIndicesBuffer;

	//Static handler of the most used game system entities,
	//Gets initialised by the user with the Init static function
	class Root
	{
	public:
		static void SetGameWindow(Window* game_window_);
		static void SetGameCamera(Camera* game_camera_);
		static void SetBlockShader(std::shared_ptr<Shader> block_shader_);
		static void SetDepthShader(std::shared_ptr<Shader> depth_shader_);
		static void SetBlockVM(std::shared_ptr<VertexManager> vertex_manager_);
		static void SetDepthVM(std::shared_ptr<VertexManager> vertex_manager_);
		static void SetShadowFramebuffer(std::shared_ptr<FrameBuffer> shadow_framebuffer_);
		static Window& GameWindow();
		static Camera& GameCamera();
		static std::shared_ptr<Shader> BlockShader();
		static std::shared_ptr<Shader> DepthShader();
		static std::shared_ptr<VertexManager> BlockVM();
		static std::shared_ptr<VertexManager> DepthVM();
		static std::shared_ptr<FrameBuffer> DepthFramebuffer();
	private:
		Root();
		~Root();
		struct RootImpl
		{
			Window* game_window;
			Camera* camera;
			std::shared_ptr<Shader> block_shader;
			std::shared_ptr<Shader> depth_shader;
			std::shared_ptr<VertexManager> block_vm;
			std::shared_ptr<VertexManager> depth_vm;
			std::shared_ptr<FrameBuffer> shadow_framebuffer;
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
		//Chunk optimized rendering
		static void RenderInstanced(uint32_t count);
	private:
		Renderer();
		~Renderer();
		void IRender(const RendererPayload& p);
		void IRenderInstanced(uint32_t count);
	};
}

//Defining types related to multithreading
template<class T>
using VecType = typename Utils::ConditionalVector<T, GlCore::g_MultithreadedRendering>;
template<class T>
using ListType = typename Utils::ConditionalList<T, GlCore::g_MultithreadedRendering>;