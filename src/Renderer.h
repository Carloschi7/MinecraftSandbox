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
	static constexpr uint32_t g_MaxRenderedObjCount = 6000;
	static constexpr uint32_t g_MaxWaterLayersCount = 500;
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

	//For debug purposes
	extern uint32_t g_Drawcalls;

	void DispatchBlockRendering(glm::vec3* position_buf, uint32_t* texture_buf, uint32_t& count);
	void DispatchDepthRendering(glm::vec3* position_buf, uint32_t& count);

	class Renderer
	{
		class Fetcher;
	public:
		static void Init();
		static Renderer& GetInstance();
		//Single instance rendering(used for cubemap and crossaim)
		static void Render(std::shared_ptr<Shader> shd, const VertexManager& vm, std::shared_ptr<CubeMap> cubemap, const glm::mat4& model);
		//Chunk optimized rendering
		static void RenderInstanced(uint32_t count);
		static void WaitForAsyncGpu(const std::vector<GLsync>& fences);
	private:
		Renderer();
		~Renderer();
		void IRender(std::shared_ptr<Shader> shd, const VertexManager& vm, std::shared_ptr<CubeMap> cubemap, const glm::mat4& model);
		void IRenderInstanced(uint32_t count);
		void IWaitForAsyncGpu(const std::vector<GLsync>& fences);
	};
}