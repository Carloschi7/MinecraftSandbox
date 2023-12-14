# MinecraftSandbox

![FramePicture](https://github.com/Carloschi7/MinecraftSandbox/assets/92332381/684dfa01-ec32-46f2-a490-10152f57fee5)


This is a project that i decided to undertake some time ago to improve my knowledge in the hard but delightful world
of graphics programming, using as always my very trustworthy C++ and OpenGL as the graphic API that allows direct
communication with the GPU

## Presentation

This application is a very simple and basic demo directly inspired by Minecraft. What was implemented was some basic generation of the
world, limiting myself to using only two biomes (default grasslands and the desert) with also some lakes and different layers of water.
Also some basic crafting was implemented, not because i wanted to offer a big variety of objects to craft, but to get a better understanding
how to approach HUD or UI desing from scratch using directly OpenGL to draw the canvas instead of using other third party libraries.
Some basic object can still be crafted, like basic wood planks and sticks and wooden pickaxes, with their recipes matching the original minecraft one

From an engineering perspective, the goal with this project was to make the most optimized application i could given that the number of objects to draw in the scene
is very large. The application is by no means the most well-optimized but, by using techniques like instanced rendering, a kind of render that allows a cheaper
rendering cost if the entities which are being drawn share the same properties that is meant to speed up the
rendering process significantly. The instanced rendering obviously ignores the blocks which cannot be seen by the player, like blocks which are hidden below 
other blocks or blocks which find themselves behind the player in that specific frame. Honestly i know this is very good for performance but the rendering
should still be faster considering that the original Minecraft rendering is way faster, but i currently do not know how to make the application any faster
from a purely OpenGL standpoint.

## Application rendering

Two main renderpasses are being computed: the default one and the depth one. The depth one is used to dynamically compute shadows in the world scene.
It is implemented with a viewpoint that from above the player watches the scene from the top. By rendering every object, the "depth camera" can see 
and depth texture is produced. That texture is a texture with the dimensions of the depth viewport, with each pixel containing information on which is
the depth value at a given point. The more the value approaches to 0.0f (meaning the more the produces texture approaches a black color), the nearer the
entity at that point. This texture then gets transferred to the main shader that uses the freshly computed values as a mean to understand if the specific
pixel is shadowed or not. The depth texture is not computed every frame as that would be pointless and the texture is large enough to cover a pretty large area
from the player. Instead the texture is calculated when the player goes far enough from the last texture computation or if the scene is altered, 
for example when a block is destroyed by the player.
The other renderpass is the actual render of the scene and the UI objects. The worlds itself with all of its blocks and water layer and trees gets initially
drawn to its own actual framebuffer. Then the computed texture gets finally drawn to the main frame with a little offset in the negative z axis to make space for 
some 3d ui elements like the block which the player holds from the inventory. Then the held block, the crossaim and the UI elements get drawn on top of it.

## Terrain generation

The world is generated using the Perlin terrain generation to generate everything. Two interlaced perlin maps are used to generate the land and another one
is used to generate the water areas. The area that is allocated to water by its perlin map is then warped downwards to create enough space to actually for the
water. The warping is done by takin the original output of the land maps and by decreasing the height of the blocks linearly using some exponential functions

## UI and crafting

The UI was made manually, with a very rudiemtary but effective grid based handler that helps the player to insert and select stuff from the inventory.
The implemented UIs are the normal player inventory and the crafting table view.

Only the following items can be currently crafted:

~~~
-> Wood planks
-> Wood sticks
-> Crafting table
-> Wooden pickaxe
~~~

Note that none of the held items, including the pickaxe, will provide any speedup effect when digging or destroying blocks, as the block durability feature
was not implemented in this demo. Also, there are no stone blocks in the current implementation, the pickaxe was only added as a way to render also 2d decals
in the hand of the player, like also the wood sticks

## Conclusion

This being said, you can still explore a potentially different and fairly simple generated world in a manually written demo.
This demo is engineless, meaning that to write this demo no external game engine was used. The only external libraries used are
the C++ standard library, glfw and glew as simple OpenGL plugins and stbi as an image loader for textures.
The actual application logic for the actual game, the game rendering and all the management of the scene objects was written
all from scratch.

This is the first major release, i may still improve this in the future by adding more contents and better terrain generation but we will see.

Enjoy
GLHF

# How to use (building or running)

This application can be built and run in both Windows and Linux. Compatibility with every linux distro is not guaranteed,
but debian distros seem to build and run the code pretty well.

## Windows
There are two ways on Windows. Either you download the release .zip, extract it and run it, or you can build everything for yourself. In order to build it from the ground up, Visual Studio 2022 + CMake + git are mandatory. Once you have all of those applications and all the environmental path set up, just clone this repo and run the build.bat. Then in Visual Studio, after having switched the compile mode to Release and after selecting the MinecraftSandbox
project as the starting project, the game will run fine. Alternatively, you can build the game using the command "build.bat standalone". That will produce an executable detached from the Visual Studio "burocracy" ready
to be run in a blank folder with the game assets and required dlls alongside it (just like in the release).

For those who just download the precompiled exe and have dll problems, just try and download the c++ development tool from the Microsoft website and try to launch the exe again

## Linux
Follow the instruction to build the C7Engine FIRST, use the packet manager to get all the required dependencies. Once the C7Engine compiles separately, then clone this repo and run build.sh, the executable should be produced.
Also, given the rather volatile nature of linux based systems, cannot guarantee that this will automatically build on every distro with the exact commands in the CMakeLists.txt of the engine. Debian-based distros should be fine
though.

If the commands in the CMakeLists.txt fail though, then some minor readjustments should do the trick. One of the possible issues that could solve the problems is changing the path bound to LINUX_LIB_PATH
to the actual path in which the required libraries like glfw,glew,assimp are being stored.
(assimp is used by the engine in the Model.h/.cpp files, i did not mention the dependency earlier because the files which use it are actually not compiled into the game, so libassimp.so like assimp.dll is not required to build this app. So up to you if you want install it or not).
