# MinecraftSandbox

This is a project that i decided to undertake some time ago to improve my knowledge in the hard but delightful world
of graphics programming, using as always my very thrusted language C++ and OpenGL as the graphic API that allows direct
communication with the GPU

This application is a very simple and basic remake of the famous Minecraft game. What was implemented was some basic generation of the
world, limiting myself to using only two chunks (default grasslands and the desert) with also some lakes and different layers of water
also some basic crafting was implemented, not because i wanted to offer a big variety of objects to craft, but to get a better understanding
how to approach HUD or UI desing from scratch using directly OpenGL to draw the canvas instead of using other third party libraries.
Some basic object can still be crafted, like basic wood planks and sticks and wooden pickaxes, with their recipes matching the original minecraft one

From an engineering perspective, the goal with this project was to make the most optimized application given that the number of objects to draw in the scene
is very large. The application is by no means the most well-optimized but, by using techniques like instanced rendering, a kind of render that allows a cheaper
rendering cost if the entities which are being drawn share the same mesh and, in a more technical sense, the same vertex array, that are meant to speed up the
rendering process significantly. The instanced rendering obviously ignores the blocks which cannot be seen by the player, like blocks which are hidden below 
other blocks or blocks which find themselves behind the player in that specific frame. Honestly i know this is very good for performance but the rendering
should still be faster considering that the original Minecraft rendering is way faster, but i currently do not know how to make the application any faster
from a purely OpenGL standpoint.

Two main renderpasses are being computed: the default one and the depth one. The depth one is used to dinamically compute shadows in the world scene.
It is implemented with a viewpoint that from above the player watches the scene from the top. By rendering every object the "depth camera" can see,
a depth texture is produced. That texture is a texture with the dimensions of the depth viewport, with each pixel containing information on which is
the depth value at a given point. The more the value approaches to 0.0f (meaning the more the produces texture approaches a black color), the nearer the
entity at that point. This texture then gets transferred to the main shader that uses the freshly computed values as a mean to understand if the specific
pixel is shadowed or not. The depth texture is not computed every frame as that would be pointless and the texture is large enough to cover a pretty large area
from the player. Instead the texture is calculated when the player goes far enough from the last texture computation or if the scene is altered, 
for example when a block is destroyed by the player.
The other renderpass is the actual render of the scene and the UI objects. The worlds itself with all of its blocks and water layer and trees gets initially
draw to its own actual framebuffer. Then the computed texture gets finally drawn to the main frame with a little offset in the negative z axis to make space for 
some 3d ui elements like the block which the player holds from the inventory. Then the held block, the crossaim and the UI elements get drawn on top of it.

The world is generated using the Perlin terrain generation to generate everything. Two interlaced perlin maps are used to generate the land and another one
is used to generate the water areas. The area that is allocated to water by its perlin map is then warped downwards to create enough space to actually for the
water. The warping is done by takin the original output of the land maps and by decreasing the height of the blocks linearly using some exponential functions

The UI was made manually, with a very rudiemtary but effective grid based handler that helps the player to insert and select stuff from the inventory.
The implemented UIs are the normal player inventory and the crafting table view.

Only the following items can be currently crafted:
-Wood planks
-Wood sticks
-Crafting table
-Wooden pickaxe

Note that none of the held items, including the pickaxe, will provide any speedup effect when digging or destroying blocks, as the block durability feature
was not implemented in this demo. Also, there are no stone blocks in the current implementation, the pickaxe was only added as a way to show this mini-engine
i wrote can also handle 2d decal view of objects that are not blocks.

This being said, you can still explore a potentially different and fairly simple generated world, and overall enjoy a pretty simple but integral version
of the very very basic minecraft aspects in a manually written demo. Because, as I already mentioned, this demo is engineless, with the engine being some
external framework that these types of applications can rely on (This demo still relies on an external lib engine library still, which is the C7Engine, 
but it's actually a simple OpenGL wrapper that I wrote, so this obviously does not count).

This is the first major release, i may still improve this in the future by adding more contents and better terrain generation.

GLHF
