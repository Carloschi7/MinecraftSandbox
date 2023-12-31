#pragma once
#include <list>
#include <thread>
#include <mutex>
#include <map>
#include "MainIncl.h"
#include "GameDefinitions.h"
#include "Vertices.h"

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
	static constexpr f32 g_FovDegrees = 60.0f;
	static constexpr u32 g_MaxRenderedObjCount = 200000;
	static constexpr u32 g_MaxWaterLayersCount = 200000;
	static constexpr u32 g_DepthMapWidth = 1024;
	static constexpr u32 g_DepthMapHeight = 1024;
	extern std::atomic_bool g_LogicThreadShouldRun;
	//thread id of the thread which is allowed to modify m_Chunks
	extern std::atomic_bool g_SerializationRunning;
	//Framebuffer data
	extern glm::vec3 g_FramebufferPlayerOffset;
	extern glm::mat4 g_DepthSpaceMatrix;

	void DispatchBlockRendering(glm::vec3*& position_buf, u32*& texture_buf, u32& count);
	void DispatchDepthRendering(glm::vec3*& position_buf, u32& count);

	class Renderer
	{
	public:
		static void Init();
		static Renderer& GetInstance();
		//Single instance rendering(used for cubemap and crossaim)
		static void Render(Shader* shd, const VertexManager& vm, CubeMap* cubemap, const glm::mat4& model);
		//Chunk optimized rendering
		static void RenderInstanced(u32 count);
	private:
		Renderer();
		~Renderer();
		void IRender(Shader* shd, const VertexManager& vm, CubeMap* cubemap, const glm::mat4& model);
		void IRenderInstanced(u32 count);
	};
}