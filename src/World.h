#pragma once
#include <iostream>
#include <optional>
#include <thread>
#include <future>
#include <random>
#include "Chunk.h"

class Inventory;

class World {
public:
    World();
    ~World();
    //Renders visible world
    void Render(const glm::vec3& camera_position, const glm::vec3& camera_direction);
    void UpdateScene(Inventory& inventory, f32 elapsed_time);
    void HandleSelection(Inventory& inventory, const glm::vec3& camera_position, const glm::vec3& camera_direction);
    void CheckPlayerCollision(const glm::vec3& position);
    //Pushes setion data to eventually help with serialization
    void HandleSectionData();
    void PushWaterLayer(std::shared_ptr<std::vector<glm::vec3>> vec);

    //Returns the corresponding chunk index if exists
    std::optional<u32> IsChunk(const Chunk& chunk, const Defs::ChunkLocation& cl);
    Chunk& GetChunk(u32 index);

    Defs::WorldSeed& Seed();
    const Defs::WorldSeed& Seed() const;

    //Serialization utilities
    void SerializeSector(u32 index);
    void DeserializeSector(u32 index);

private:
    //Function which handles spawnable chunk pushing conditions
    bool IsPushable(const Chunk& chunk, const Defs::ChunkLocation& cl, const glm::vec3& vec);
    glm::vec2 SectionCentralPosFrom(u32 index);
    
public:
    //Vector which stores a the general area of all watermaps found up to that moment
    Utils::AVector<Defs::WaterArea> pushed_areas;
    Utils::AVector<glm::vec3> relative_leaves_positions;
    std::mt19937 random_engine;

private:
    //Global OpenGL environment state
    GlCore::State& m_State;
    //The AVector is a vector type that uses our allocated memory
    //arena, in this case we do not directly allocate a heap section
    //of contiguous chunks like in AVector<Chunk> because we want to be
    //able to be flexible in multithreading. We just store a vistual address
    //in our virtual memory space
    Utils::AVector<VAddr> m_Chunks;
    //Non existing chunk which are near existing ones. They can spawn if the
    //player gets near enough
    Utils::AVector<glm::vec3> m_SpawnableChunks;
    //Water layers pushed for current iteration
    std::vector<std::shared_ptr<std::vector<glm::vec3>>> m_DrawableWaterLayers;
    //Last player pos, used to update the shadow texture
    glm::vec3 m_LastPos;
    //For terrain generation
    Defs::WorldSeed m_WorldSeed;
    //Handles section data
    Utils::AVector<Defs::SectionData> m_SectionsData;  
    //Timer used to sort the spawnable chunks vector every now and then
    //(sorting every frame would be pointless)
    Utils::Timer m_SortingTimer;

    //Buffer which holds an address to a small chunk buffer that contains the chunk
    //which the player can collide with
    //Also by declaring this we minimize the amount of allocation per frame
    //in the renderer thread
    VAddr m_CollisionChunkBuffer;
    VAddr m_RemovableChunkBuffer;

    //Serialization threads
    std::future<void> m_SerializingFut;
};