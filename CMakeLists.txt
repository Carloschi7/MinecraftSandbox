cmake_minimum_required(VERSION 3.12)

if("${APP_NAME}" STREQUAL "")
	set(APP_NAME Application)
endif()

if("${ENGINE_PATH}" STREQUAL "" OR "${ENGINE_BUILD_PATH}" STREQUAL "")
	message(FATAL_ERROR "You need to define the vars ENGINE_PATH and ENGINE_BUILD_PATH, Check out the Github page to see what they should be bound to")
endif()

project(${APP_NAME})

#Visual studio is only availabe on Windows, so no need to check for anything else
if(MSVC)
	#Prebuilt dependencies for Windows
	set(C7ENGINE_OPENGL_LIB opengl32.lib)
	set(C7ENGINE_DEPENDENCIES 
		${ENGINE_PATH}/Dependencies/ASSIMP/lib/assimp-vc143-mtd.lib
		${ENGINE_PATH}/Dependencies/GLEW/lib/Release/x64/glew32sd.lib
		${ENGINE_PATH}/Dependencies/GLFW/lib-vc2019/glfw3.lib)

	set(C7ENGINE_LIB_PATH ${ENGINE_BUILD_PATH}/$(Configuration)/C7Engine.lib)
elseif(LINUX)
	find_package(Threads REQUIRED)
	set(UBUNTU_LIB_PATH /usr/lib/x86_64-linux-gnu)
	set(C7ENGINE_OPENGL_LIB ${UBUNTU_LIB_PATH}/libGL.so)
	set(C7ENGINE_DEPENDENCIES glfw GLEW ${UBUNTU_LIB_PATH}/libassimp.so)
	set(C7ENGINE_LIB_PATH  ${ENGINE_BUILD_PATH}/libC7Engine.so Threads::Threads)

	#Filesystem support
	link_libraries(stdc++fs)
else()
	message(FATAL_ERROR "This OS is currently not supported")
endif()

#The .h files normally are not mandatory to be added here, but in this case adding them
#tells Visual Studio to show them in the solution explorer
add_executable(${PROJECT_NAME} src/Main.cpp
		src/Application.cpp
		src/World.cpp src/World.h
		src/Chunk.cpp src/Chunk.h
		src/Block.cpp src/Block.h
		src/GlStructure.cpp src/GlStructure.h
		src/Renderer.h src/Renderer.cpp
		src/InventorySystem.h src/InventorySystem.cpp
		src/GameDefinitions.h src/GameDefinitions.cpp
		src/Memory.h src/Memory.cpp
		src/State.h src/State.cpp
		src/Macros.h
		src/Utils.h src/Utils.cpp
		src/Vertices.h src/Vertices.cpp
		)

target_link_libraries(${PROJECT_NAME} ${C7ENGINE_OPENGL_LIB} ${C7ENGINE_DEPENDENCIES} ${C7ENGINE_LIB_PATH})

target_include_directories(${PROJECT_NAME} PUBLIC
	${ENGINE_PATH}/engine 
	${ENGINE_PATH}/Dependencies/ASSIMP/include
	${ENGINE_PATH}/Dependencies/GLM
	${ENGINE_PATH}/Dependencies/stbi
	${ENGINE_PATH}/Dependencies/GLFW
	${ENGINE_PATH}/Dependencies/GLEW)

if(DEFINED STANDALONE)
	#This is the build that creates an executable meant to be shipped as a standalone app
	#this means assets will be loaded directly from the assets data in the same folder as the executable
	target_compile_definitions(${PROJECT_NAME} PRIVATE MC_SOURCE_PATH="")
	target_compile_definitions(${PROJECT_NAME} PRIVATE MC_STANDALONE)
else()
	target_compile_definitions(${PROJECT_NAME} PRIVATE MC_SOURCE_PATH="${CMAKE_CURRENT_LIST_DIR}/")
endif()
#Tell the solution that we dont want to use assimp for this project
target_compile_definitions(${PROJECT_NAME} PRIVATE NO_ASSIMP)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
