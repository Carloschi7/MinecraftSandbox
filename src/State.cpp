#include "State.h"

#include "VertexManager.h"
#include "Window.h"
#include "Camera.h"
#include "Shader.h"
#include "FrameBuffer.h"

namespace GlCore
{
	State::State() :
		camera(nullptr), game_window(nullptr)
	{
	}
	State::~State()
	{
	}

	State* pstate = nullptr;
}

