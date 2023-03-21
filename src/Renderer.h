#pragma once
#include <memory>
#include <list>
#include <thread>
#include <mutex>
#include <map>
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
#ifdef MC_MULTITHREADING
	static constexpr bool g_MultithreadedRendering = true;
#else
	static constexpr bool g_MultithreadedRendering = false;
#endif
	static constexpr uint32_t g_MaxInstancedObjs = 1500;
	static constexpr uint32_t g_DepthMapWidth = 1024;
	static constexpr uint32_t g_DepthMapHeight = 1024;
	extern std::atomic_bool g_LogicThreadShouldRun;
	//thread id of the thread which is allowed to modify m_Chunks
	extern std::atomic_bool g_SerializationRunning;
	extern std::map<std::string, std::thread::id> g_ThreadPool;
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