cmake_minimum_required(VERSION 3.12)

if("${APP_NAME}" STREQUAL "")
	set(APP_NAME Application)
endif()

string(SUBSTRING "${CMAKE_GENERATOR}" 0 13 VISUAL_STUDIO_DEF)

if("${ENGINE_PATH}" STREQUAL "" OR "${ENGINE_BUILD_PATH}" STREQUAL "")
	message(FATAL_ERROR "You need to define the vars ENGINE_PATH and ENGINE_BUILD_PATH, Check out the Github page to see what they should be bound to")
endif()

project(${APP_NAME})

#Visual studio is only availabe on Windows, so no need to check for anything else
#On 
if("${VISUAL_STUDIO_DEF}" STREQUAL "Visual Studio")
	#Prebuilt dependencies for Windows

	set(C7ENGINE_OPENGL_LIB opengl32.lib)

	set(C7ENGINE_DEPENDENCIES 
		${ENGINE_PATH}/Dependencies/ASSIMP/lib/assimp-vc143-mtd.lib
		${ENGINE_PATH}/Dependencies/GLEW/lib/Release/x64/glew32sd.lib
		${ENGINE_PATH}/Dependencies/GLFW/lib-vc2019/glfw3.lib)

	set(C7ENGINE_LIB_PATH ${ENGINE_BUILD_PATH}/$(Configuration)/C7Engine.lib)

else() #default linux ubuntu for now (STILL WORK IN PROGRESS)
	find_package(Threads REQUIRED)
	set(UBUNTU_LIB_PATH /usr/lib/x86_64-linux-gnu)
	set(C7ENGINE_OPENGL_LIB ${UBUNTU_LIB_PATH}/libGL.so)
	set(C7ENGINE_DEPENDENCIES glfw GLEW ${UBUNTU_LIB_PATH}/libassimp.so)
	set(C7ENGINE_LIB_PATH  ${ENGINE_BUILD_PATH}/libC7Engine.so Threads::Threads)

	#Filesystem support
	link_libraries(stdc++fs)
endif()


add_executable(${PROJECT_NAME} src/Main.cpp
		src/Application.cpp
		src/World.cpp src/World.h
		src/Chunk.cpp src/Chunk.h
		src/Block.cpp src/Block.h
		src/GlStructure.cpp src/GlStructure.h
		src/Renderer.h src/Renderer.cpp
		src/InventorySystem.h src/InventorySystem.cpp
		src/GameDefinitions.h src/GameDefinitions.cpp
		src/State.h src/State.cpp
		src/Macros.h
		src/Utils.h
		src/Vertices.h
		)

target_link_libraries(${PROJECT_NAME} ${C7ENGINE_OPENGL_LIB} ${C7ENGINE_DEPENDENCIES} ${C7ENGINE_LIB_PATH})

target_include_directories(${PROJECT_NAME} PUBLIC
	${ENGINE_PATH}/engine 
	${ENGINE_PATH}/Dependencies/ASSIMP/include
	${ENGINE_PATH}/Dependencies/GLM
	${ENGINE_PATH}/Dependencies/stbi
	${ENGINE_PATH}/Dependencies/GLFW
	${ENGINE_PATH}/Dependencies/GLEW)

if("${VISUAL_STUDIO_DEF}" STREQUAL "Visual Studio")
	#Pasting shared library into build dir, necessary on Windows for our current setup
	file(COPY ${ENGINE_PATH}/Dependencies/ASSIMP/bin/assimp-vc143-mtd.dll DESTINATION ${PROJECT_BINARY_DIR})
endif()

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/assets/shaders)
file(COPY assets/textures DESTINATION assets)
file(COPY assets/shaders DESTINATION assets)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
